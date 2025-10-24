/****************************************************************************
*
*    Copyright (c) 2019  by amlogic Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of amlogic Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of amlogic Corporation.
*
***************************************************************************/
/*-------------------------------------------
                Includes
-------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <float.h>

#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <array>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#include "nn_sdk.h"

///////////////////////////////////////////////////////////
static std::vector<std::string> classes = {"person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
                                           "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
                                           "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
                                           "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
                                           "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
                                           "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "doughnut", "cake", "chair", "couch",
                                           "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard",
                                           "cell phone", "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase",
                                           "scissors", "teddy bear", "hair drier", "toothbrush"};

struct PreprocessResult
{
    std::vector<uint8_t> image;
    float scale;
    int pad_left;
    int pad_top;
};

struct FeatureMap
{
    float *data;
    int height;
    int width;
    int channels;
    int stride;
};

struct Detection
{
    float x1, y1, x2, y2;  // Bounding box coordinates
    float score;           // Confidence score
    int class_id;          // Predicted class ID
};

struct DrawDetection
{
    int x1;
    int y1;
    int x2;
    int y2;
    std::string label;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    int thickness;
};

static inline uint8_t clamp_u8(int v)
{
    if (v < 0) return 0;
    if (v > 255) return 255;
    return static_cast<uint8_t>(v);
}

static inline void set_pixel(uint8_t* img, int w, int h, int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    int idx = (y * w + x) * 3;
    img[idx + 0] = r;
    img[idx + 1] = g;
    img[idx + 2] = b;
}

static void draw_hline(uint8_t* img, int w, int h, int x1, int x2, int y, uint8_t r, uint8_t g, uint8_t b)
{
    if (y < 0 || y >= h) return;
    if (x1 > x2) std::swap(x1, x2);
    x1 = std::max(0, x1);
    x2 = std::min(w - 1, x2);
    for (int x = x1; x <= x2; ++x)
    {
        set_pixel(img, w, h, x, y, r, g, b);
    }
}

static void draw_vline(uint8_t* img, int w, int h, int x, int y1, int y2, uint8_t r, uint8_t g, uint8_t b)
{
    if (x < 0 || x >= w) return;
    if (y1 > y2) std::swap(y1, y2);
    y1 = std::max(0, y1);
    y2 = std::min(h - 1, y2);
    for (int y = y1; y <= y2; ++y)
    {
        set_pixel(img, w, h, x, y, r, g, b);
    }
}

static void draw_rect(uint8_t* img, int w, int h, int x1, int y1, int x2, int y2, int thickness, uint8_t r, uint8_t g, uint8_t b)
{
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    for (int t = 0; t < thickness; ++t)
    {
        draw_hline(img, w, h, x1, x2, y1 + t, r, g, b);
        draw_hline(img, w, h, x1, x2, y2 - t, r, g, b);
        draw_vline(img, w, h, x1 + t, y1, y2, r, g, b);
        draw_vline(img, w, h, x2 - t, y1, y2, r, g, b);
    }
}

static void fill_rect(uint8_t* img, int w, int h, int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b)
{
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    x1 = std::max(0, x1);
    y1 = std::max(0, y1);
    x2 = std::min(w - 1, x2);
    y2 = std::min(h - 1, y2);
    for (int y = y1; y <= y2; ++y)
    {
        for (int x = x1; x <= x2; ++x)
        {
            set_pixel(img, w, h, x, y, r, g, b);
        }
    }
}

static bool glyph_lookup(char ch, uint8_t out[7])
{
    uint8_t blank[7] = {0,0,0,0,0,0,0};
    if (ch == ' ')
    {
        memcpy(out, blank, 7);
        return true;
    }
    switch (ch)
    {
        case '-': { uint8_t g[7]={0x00,0x00,0x00,0x1F,0x00,0x00,0x00}; memcpy(out,g,7); return true; }
        case '.': { uint8_t g[7]={0x00,0x00,0x00,0x00,0x00,0x06,0x06}; memcpy(out,g,7); return true; }
        case '/': { uint8_t g[7]={0x01,0x01,0x02,0x04,0x08,0x10,0x10}; memcpy(out,g,7); return true; }
        case '%': { uint8_t g[7]={0x19,0x19,0x02,0x04,0x08,0x13,0x13}; memcpy(out,g,7); return true; }
        default: break;
    }
    if (ch >= '0' && ch <= '9')
    {
        static const uint8_t digits[][7] = {
            {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},
            {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
            {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F},
            {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E},
            {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
            {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
            {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},
            {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
            {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
            {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}
        };
        memcpy(out, digits[ch - '0'], 7);
        return true;
    }

    switch (ch)
    {
        case 'A': { uint8_t g[7]={0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}; memcpy(out,g,7); return true; }
        case 'B': { uint8_t g[7]={0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}; memcpy(out,g,7); return true; }
        case 'C': { uint8_t g[7]={0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}; memcpy(out,g,7); return true; }
        case 'D': { uint8_t g[7]={0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}; memcpy(out,g,7); return true; }
        case 'E': { uint8_t g[7]={0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}; memcpy(out,g,7); return true; }
        case 'F': { uint8_t g[7]={0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}; memcpy(out,g,7); return true; }
        case 'G': { uint8_t g[7]={0x0E,0x11,0x10,0x17,0x11,0x11,0x0E}; memcpy(out,g,7); return true; }
        case 'H': { uint8_t g[7]={0x11,0x11,0x11,0x1F,0x11,0x11,0x11}; memcpy(out,g,7); return true; }
        case 'I': { uint8_t g[7]={0x1F,0x04,0x04,0x04,0x04,0x04,0x1F}; memcpy(out,g,7); return true; }
        case 'J': { uint8_t g[7]={0x1F,0x02,0x02,0x02,0x12,0x12,0x0C}; memcpy(out,g,7); return true; }
        case 'K': { uint8_t g[7]={0x11,0x12,0x14,0x18,0x14,0x12,0x11}; memcpy(out,g,7); return true; }
        case 'L': { uint8_t g[7]={0x10,0x10,0x10,0x10,0x10,0x10,0x1F}; memcpy(out,g,7); return true; }
        case 'M': { uint8_t g[7]={0x11,0x1B,0x15,0x15,0x11,0x11,0x11}; memcpy(out,g,7); return true; }
        case 'N': { uint8_t g[7]={0x11,0x19,0x15,0x13,0x11,0x11,0x11}; memcpy(out,g,7); return true; }
        case 'O': { uint8_t g[7]={0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}; memcpy(out,g,7); return true; }
        case 'P': { uint8_t g[7]={0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}; memcpy(out,g,7); return true; }
        case 'Q': { uint8_t g[7]={0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}; memcpy(out,g,7); return true; }
        case 'R': { uint8_t g[7]={0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}; memcpy(out,g,7); return true; }
        case 'S': { uint8_t g[7]={0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}; memcpy(out,g,7); return true; }
        case 'T': { uint8_t g[7]={0x1F,0x04,0x04,0x04,0x04,0x04,0x04}; memcpy(out,g,7); return true; }
        case 'U': { uint8_t g[7]={0x11,0x11,0x11,0x11,0x11,0x11,0x0E}; memcpy(out,g,7); return true; }
        case 'V': { uint8_t g[7]={0x11,0x11,0x11,0x11,0x0A,0x0A,0x04}; memcpy(out,g,7); return true; }
        case 'W': { uint8_t g[7]={0x11,0x11,0x11,0x15,0x15,0x1B,0x11}; memcpy(out,g,7); return true; }
        case 'X': { uint8_t g[7]={0x11,0x0A,0x04,0x04,0x04,0x0A,0x11}; memcpy(out,g,7); return true; }
        case 'Y': { uint8_t g[7]={0x11,0x11,0x0A,0x04,0x04,0x04,0x04}; memcpy(out,g,7); return true; }
        case 'Z': { uint8_t g[7]={0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}; memcpy(out,g,7); return true; }
        default: break;
    }
    return false;
}

static void draw_char5x7(uint8_t* img, int w, int h, int x, int y, char ch, uint8_t r, uint8_t g, uint8_t b, int scale)
{
    if (ch >= 'a' && ch <= 'z')
    {
        ch = static_cast<char>(ch - 'a' + 'A');
    }
    uint8_t rows[7];
    if (!glyph_lookup(ch, rows))
    {
        return;
    }
    for (int row = 0; row < 7; ++row)
    {
        uint8_t bits = rows[row];
        for (int col = 0; col < 5; ++col)
        {
            if (bits & (1 << (4 - col)))
            {
                for (int sy = 0; sy < scale; ++sy)
                {
                    for (int sx = 0; sx < scale; ++sx)
                    {
                        set_pixel(img, w, h, x + col * scale + sx, y + row * scale + sy, r, g, b);
                    }
                }
            }
        }
    }
}

static void draw_text(uint8_t* img, int w, int h, int x, int y, const std::string& text, uint8_t r, uint8_t g, uint8_t b, int scale)
{
    int cx = x;
    for (char ch : text)
    {
        draw_char5x7(img, w, h, cx, y, ch, r, g, b, scale);
        cx += 6 * scale;
    }
}

static void draw_label(uint8_t* img, int w, int h, int x, int y, const std::string& text, uint8_t br, uint8_t bg, uint8_t bb, int scale)
{
    int text_w = static_cast<int>(text.size()) * 6 * scale - scale;
    if (text_w < 0) text_w = 0;
    int text_h = 7 * scale;
    int pad = 2 * scale;
    int x2 = x + pad + text_w + pad;
    int y2 = y + pad + text_h + pad;
    fill_rect(img, w, h, x, y, x2, y2, br, bg, bb);
    draw_text(img, w, h, x + pad, y + pad, text, 255, 255, 255, scale);
}

static void render_detections(uint8_t* img, int w, int h, const std::vector<DrawDetection>& dets)
{
    int base = std::min(w, h);
    int scale = std::max(1, std::min(8, base / 200));
    for (const auto& det : dets)
    {
        int x1 = std::max(0, std::min(w - 1, det.x1));
        int y1 = std::max(0, std::min(h - 1, det.y1));
        int x2 = std::max(0, std::min(w - 1, det.x2));
        int y2 = std::max(0, std::min(h - 1, det.y2));
        int thick = std::max(1, det.thickness * scale / 2);
        draw_rect(img, w, h, x1, y1, x2, y2, thick, det.r, det.g, det.b);
        int label_h = 7 * scale + 4 * scale;
        int label_y = std::max(0, y1 - label_h);
        draw_label(img, w, h, x1, label_y, det.label, det.r, det.g, det.b, scale);
    }
}

static const std::array<std::array<uint8_t, 3>, 8> BOX_COLORS = {{
    {255, 0, 0},
    {0, 128, 255},
    {0, 200, 70},
    {255, 140, 0},
    {185, 40, 255},
    {0, 220, 220},
    {255, 215, 0},
    {80, 200, 120}
}};


static PreprocessResult preprocess(const unsigned char *src, int src_w, int src_h, int channels, int dst_h, int dst_w)
{
    PreprocessResult result;
    result.scale = 1.0f;
    result.pad_left = 0;
    result.pad_top = 0;

    if (!src || src_w <= 0 || src_h <= 0 || channels <= 0 || dst_h <= 0 || dst_w <= 0)
    {
        return result;
    }

    float scale_h = static_cast<float>(dst_h) / static_cast<float>(src_h);
    float scale_w = static_cast<float>(dst_w) / static_cast<float>(src_w);
    float scale = std::min(scale_h, scale_w);
    int new_h = static_cast<int>(roundf(static_cast<float>(src_h) * scale));
    int new_w = static_cast<int>(roundf(static_cast<float>(src_w) * scale));

    if (new_h <= 0 || new_w <= 0)
    {
        return result;
    }

    std::vector<uint8_t> resized(new_w * new_h * channels);
    unsigned char *resized_ptr = stbir_resize_uint8_linear(src, src_w, src_h, 0,
                                                           resized.data(), new_w, new_h, 0,
                                                           static_cast<stbir_pixel_layout>(channels));
    if (!resized_ptr)
    {
        printf("stbir_resize_uint8 failed, fallback to simple copy.\n");
        size_t src_bytes = static_cast<size_t>(src_w) * static_cast<size_t>(src_h) * static_cast<size_t>(channels);
        size_t copy_bytes = std::min(resized.size(), src_bytes);
        memcpy(resized.data(), src, copy_bytes);
    }

    int pad_h = dst_h - new_h;
    int pad_w = dst_w - new_w;
    int pad_left = static_cast<int>(roundf(pad_w / 2.0f - 0.1f));
    int pad_right = static_cast<int>(roundf(pad_w / 2.0f + 0.1f));
    int pad_top = static_cast<int>(roundf(pad_h / 2.0f - 0.1f));
    int pad_bottom = static_cast<int>(roundf(pad_h / 2.0f + 0.1f));

    std::vector<uint8_t> letterbox(dst_w * dst_h * channels, 114);
    for (int y = 0; y < new_h; ++y)
    {
        uint8_t *dst_row = letterbox.data() + ((pad_top + y) * dst_w + pad_left) * channels;
        const uint8_t *src_row = resized.data() + y * new_w * channels;
        memcpy(dst_row, src_row, static_cast<size_t>(new_w) * channels);
    }

    result.image = std::move(letterbox);
    result.scale = scale;
    result.pad_left = pad_left;
    result.pad_top = pad_top;
    return result;
}

static float compute_iou(const Detection& det1, const Detection& det2)
{
    float xx1 = std::max(det1.x1, det2.x1);
    float yy1 = std::max(det1.y1, det2.y1);
    float xx2 = std::min(det1.x2, det2.x2);
    float yy2 = std::min(det1.y2, det2.y2);

    float w = std::max(0.0f, xx2 - xx1);
    float h = std::max(0.0f, yy2 - yy1);
    float inter = w * h;

    float area1 = (det1.x2 - det1.x1) * (det1.y2 - det1.y1);
    float area2 = (det2.x2 - det2.x1) * (det2.y2 - det2.y1);

    return inter / (area1 + area2 - inter);
}

static std::vector<Detection> nms_by_class(const std::vector<Detection>& detections, float iou_threshold)
{
    if (detections.empty()) return {};

    std::vector<Detection> final_detections;

    std::unordered_map<int, std::vector<Detection>> class_detections;
    for (const auto& det : detections)
    {
        class_detections[det.class_id].push_back(det);
    }

    for (auto& [class_id, cls_dets] : class_detections)
    {
        std::sort(cls_dets.begin(), cls_dets.end(), [](const Detection& a, const Detection& b)
        {
            return a.score > b.score;
        });

        std::vector<bool> removed(cls_dets.size(), false);
        for (size_t i = 0; i < cls_dets.size(); ++i)
        {
            if (removed[i])
            {
                continue;
            }
            final_detections.push_back(cls_dets[i]);

            for (size_t j = i + 1; j < cls_dets.size(); ++j)
            {
                if (removed[j])
                {
                    continue;
                }
                if (compute_iou(cls_dets[i], cls_dets[j]) > iou_threshold)
                {
                    removed[j] = true;
                }
            }
        }
    }

    return final_detections;
}

static float sigmoid(float x)
{
    return 1.0f / (1.0f + std::exp(-x));
}

static std::vector<Detection> get_detections(const FeatureMap& fmap, float conf_thresh, int num_classes)
{
    std::vector<Detection> detections;

    if (fmap.data == NULL || fmap.height <= 0 || fmap.width <= 0 || fmap.channels <= 0 || fmap.stride <= 0)
    {
        return detections;
    }

    int grid_h = fmap.height;
    int grid_w = fmap.width;
    int channel_stride = fmap.channels;
    int dfl_offset = 0;
    int cls_offset = dfl_offset + 4 * 16;

    if (channel_stride < cls_offset + num_classes)
    {
        return detections;
    }

    for (int i = 0; i < grid_h; ++i)
    {
        for (int j = 0; j < grid_w; ++j)
        {
            int idx = (i * grid_w + j) * channel_stride;

            float max_score = -1.0f;
            int class_id = -1;
            for (int c = 0; c < num_classes; ++c)
            {
                float score = sigmoid(fmap.data[idx + cls_offset + c]);
                if (score > max_score)
                {
                    max_score = score;
                    class_id = c;
                }
            }

            if (max_score < conf_thresh)
            {
                continue;
            }

            float exp_vals[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            for (int k = 0; k < 4; ++k)
            {
                int dfl_idx = idx + dfl_offset + k * 16;
                float exp_logits[16];
                float sum_exp = 0.0f;

                float max_logit = fmap.data[dfl_idx];
                for (int t = 1; t < 16; ++t)
                {
                    if (fmap.data[dfl_idx + t] > max_logit)
                    {
                        max_logit = fmap.data[dfl_idx + t];
                    }
                }

                for (int t = 0; t < 16; ++t)
                {
                    exp_logits[t] = std::exp(fmap.data[dfl_idx + t] - max_logit);
                    sum_exp += exp_logits[t];
                }

                for (int t = 0; t < 16; ++t)
                {
                    exp_logits[t] /= sum_exp;
                    exp_vals[k] += t * exp_logits[t];
                }
            }

            float x1 = (j + 0.5f - exp_vals[0]) * fmap.stride;
            float y1 = (i + 0.5f - exp_vals[1]) * fmap.stride;
            float x2 = (j + 0.5f + exp_vals[2]) * fmap.stride;
            float y2 = (i + 0.5f + exp_vals[3]) * fmap.stride;

            detections.push_back({x1, y1, x2, y2, max_score, class_id});
        }
    }

    return detections;
}

std::vector<Detection> postprocess(const std::array<FeatureMap, 3>& feature_maps,
                                   float scale, int pad_left, int pad_top,
                                   float conf_thresh, float iou_threshold, int num_classes)
{
    if (scale <= 0.0f)
    {
        scale = 1.0f;
    }

    std::vector<Detection> detections;

    for (const auto& fmap : feature_maps)
    {
        std::vector<Detection> layer = get_detections(fmap, conf_thresh, num_classes);
        detections.insert(detections.end(), layer.begin(), layer.end());
    }

    std::vector<Detection> detections_orig;
    for (const auto& det : detections)
    {
        float x1_orig = (det.x1 - static_cast<float>(pad_left)) / scale;
        float y1_orig = (det.y1 - static_cast<float>(pad_top)) / scale;
        float x2_orig = (det.x2 - static_cast<float>(pad_left)) / scale;
        float y2_orig = (det.y2 - static_cast<float>(pad_top)) / scale;
        detections_orig.push_back({x1_orig, y1_orig, x2_orig, y2_orig, det.score, det.class_id});
    }


    std::vector<Detection> detections_nms;
    detections_nms = nms_by_class(detections_orig, iou_threshold);

    return detections_nms;
}


int main(int argc,char **argv)
{
    if (argc < 4)
    {
        printf("Usage:\n");
        printf("argv[0]: exe_file\n");
        printf("argv[1]: xxx.tflite\n");
        printf("argv[2]: xxx.jpg/jpeg/png\n");
        printf("argv[3] (optional): 0(cpu) | 1(gpu)\n");
        return -1;
    }

    int ret = 0;

    aml_hw_flag_t hw_flag = AML_HW_CPU;
    if (atoi(argv[3]) == 0) // 0: CPU, 1: GPU via CLI
    {
        hw_flag = AML_HW_CPU;
    }
    else if (atoi(argv[3]) == 1)
    {
        hw_flag = AML_HW_GPU;
    }


    void *qcontext = NULL;
    aml_config config;
    memset(&config, 0, sizeof(aml_config));
    config.path = argv[1];
    config.modelType = TENSORFLOWLITE;
    config.hw_flag = hw_flag;
    qcontext = aml_module_create(&config);
    if (NULL == qcontext)
    {
        printf("aml_module_create fail.\n");
        return -1;
    }

    printf("Use %s backend.\n", hw_flag == AML_HW_GPU ? "GPU" : "CPU");


    tensor_info *input_tensor;
    tensor_info *output_tensor;
    ret = aml_util_getTensorInfo(qcontext, NULL, &input_tensor, &output_tensor);
    if (ret)
    {
        printf("aml_util_getTensorInfo fail.\n");
        return -1;
    }

    printf("\ninput tensors: %d\n", input_tensor->num);
    printf("output tensors: %d\n", output_tensor->num);
    for (int i = 0; i < input_tensor->num; i++)
    {
        printf("input[%d], shape=[%d,%d,%d,%d], zp=%d, scale=%f, type=%d\n", 
                i, 
                input_tensor->info[i].sizes_of_dim[0], 
                input_tensor->info[i].sizes_of_dim[1], 
                input_tensor->info[i].sizes_of_dim[2], 
                input_tensor->info[i].sizes_of_dim[3], 
                input_tensor->info[i].TF_zeropoint, 
                input_tensor->info[i].TF_scale, 
                input_tensor->info[i].data_format);
    }
    for (int i = 0; i < output_tensor->num; i++)
    {
        printf("output[%d], shape=[%d,%d,%d,%d], zp=%d, scale=%f, type=%d\n", 
                i, 
                output_tensor->info[i].sizes_of_dim[0], 
                output_tensor->info[i].sizes_of_dim[1], 
                output_tensor->info[i].sizes_of_dim[2], 
                output_tensor->info[i].sizes_of_dim[3], 
                output_tensor->info[i].TF_zeropoint, 
                output_tensor->info[i].TF_scale, 
                output_tensor->info[i].data_format);
    }


    int iw, ih, n;
    int input_size;
    unsigned char *idata = stbi_load(argv[2], &iw, &ih, &n, 0);
    std::vector<uint8_t> original_image;
    if (idata != NULL)
    {
        original_image.assign(idata, idata + static_cast<size_t>(iw) * ih * n);
    }
    int target_h = 416;
    int target_w = 416;
    PreprocessResult prep = preprocess(idata, iw, ih, n, target_h, target_w);
    stbi_image_free(idata);

    input_size = prep.image.size();
    printf("iw = %d, ih = %d, n = %d, resized=[%d,%d,%d], input_size = %d\n", iw, ih, n, target_h, target_w, n, input_size);

    nn_input inData;
    memset(&inData, 0, sizeof(nn_input));
    inData.input_index = 0;
    inData.input = prep.image.data();
    inData.size = input_size;
    inData.info.input_data_type = AML_INPUT_U8;
    inData.input_type = BINARY_RAW_DATA;
    ret = aml_module_input_set(qcontext, &inData);
    if (ret)
    {
        printf("aml_module_input_set fail.\n");
        return -1;
    }


    aml_output_config_t outconfig;
    nn_output *outdata = NULL;
    memset(&outconfig,0,sizeof(aml_output_config_t));
    outconfig.format = AML_OUTDATA_RAW;
    outdata = (nn_output*)aml_module_output_get(qcontext, outconfig);
    if (NULL == outdata)
    {
        printf("aml_module_output_get fail.\n");
        return -1;
    }

    printf("outdata->out[0].size = %d\n", outdata->out[0].size);
    printf("outdata->out[1].size = %d\n", outdata->out[1].size);
    printf("outdata->out[2].size = %d\n", outdata->out[2].size);

    auto dequantize = [](const uint8_t* src, int elements, float scale, int zp)
    {
        std::vector<float> dst(elements);
        for (int i = 0; i < elements; ++i)
        {
            dst[i] = (static_cast<int>(src[i]) - zp) * scale;
        }
        return dst;
    };

    const uint8_t* raw0 = static_cast<const uint8_t*>(outdata->out[1].buf);
    const uint8_t* raw1 = static_cast<const uint8_t*>(outdata->out[0].buf);
    const uint8_t* raw2 = static_cast<const uint8_t*>(outdata->out[2].buf);

    std::vector<float> outbuf0 = dequantize(raw0, outdata->out[1].size, output_tensor->info[1].TF_scale, output_tensor->info[1].TF_zeropoint);
    std::vector<float> outbuf1 = dequantize(raw1, outdata->out[0].size, output_tensor->info[0].TF_scale, output_tensor->info[0].TF_zeropoint);
    std::vector<float> outbuf2 = dequantize(raw2, outdata->out[2].size, output_tensor->info[2].TF_scale, output_tensor->info[2].TF_zeropoint);

    std::array<FeatureMap, 3> feature_maps = {{
        {outbuf0.data(), 52, 52, 144, 8},
        {outbuf1.data(), 26, 26, 144, 16},
        {outbuf2.data(), 13, 13, 144, 32},
    }};

    float conf_thresh = 0.35;
    float iou_threshold = 0.45;
    std::vector<Detection> detections = postprocess(feature_maps,
                                                    prep.scale, prep.pad_left, prep.pad_top,
                                                    conf_thresh, iou_threshold, classes.size());

    for (auto& det : detections)
    {
        det.x1 = std::max(0.0f, std::min(det.x1, static_cast<float>(iw)));
        det.y1 = std::max(0.0f, std::min(det.y1, static_cast<float>(ih)));
        det.x2 = std::max(0.0f, std::min(det.x2, static_cast<float>(iw)));
        det.y2 = std::max(0.0f, std::min(det.y2, static_cast<float>(ih)));
    }

    if (detections.empty())
    {
        printf("No detections above threshold.\n");
    }
    else
    {
        printf("Detections (count=%zu):\n", detections.size());
        for (const auto& det : detections)
        {
            const char* cls_name = (det.class_id >= 0 && det.class_id < static_cast<int>(classes.size())) ? classes[det.class_id].c_str() : "unknown";
            printf("  class=%s (id=%d) score=%.3f box=[%.1f, %.1f, %.1f, %.1f]\n",
                   cls_name, det.class_id, det.score, det.x1, det.y1, det.x2, det.y2);
        }
    }

    if (!detections.empty() && !original_image.empty())
    {
        std::vector<DrawDetection> draw_jobs;
        draw_jobs.reserve(detections.size());
        for (const auto& det : detections)
        {
            if (det.x2 <= det.x1 || det.y2 <= det.y1)
            {
                continue;
            }
            const char* cls_name = (det.class_id >= 0 && det.class_id < static_cast<int>(classes.size())) ? classes[det.class_id].c_str() : "unknown";
            char label_buf[128];
            snprintf(label_buf, sizeof(label_buf), "%s %.2f", cls_name, det.score);
            size_t color_idx = (det.class_id >= 0) ? static_cast<size_t>(det.class_id) % BOX_COLORS.size() : 0;
            draw_jobs.push_back({
                static_cast<int>(std::round(det.x1)),
                static_cast<int>(std::round(det.y1)),
                static_cast<int>(std::round(det.x2)),
                static_cast<int>(std::round(det.y2)),
                std::string(label_buf),
                BOX_COLORS[color_idx][0],
                BOX_COLORS[color_idx][1],
                BOX_COLORS[color_idx][2],
                2
            });
        }

        if (!draw_jobs.empty())
        {
            render_detections(original_image.data(), iw, ih, draw_jobs);
            std::string out_path = argv[2];
            size_t dot_pos = out_path.find_last_of('.');
            size_t slash_pos = out_path.find_last_of("/\\");
            if (dot_pos == std::string::npos || (slash_pos != std::string::npos && dot_pos < slash_pos))
            {
                out_path += "_det.jpg";
            }
            else
            {
                out_path = out_path.substr(0, dot_pos) + "_det.jpg";
            }
            if (stbi_write_jpg(out_path.c_str(), iw, ih, 3, original_image.data(), 90))
            {
                printf("Annotated image saved to %s\n", out_path.c_str());
            }
            else
            {
                printf("Failed to save annotated image: %s\n", out_path.c_str());
            }
        }
    }


    ret = aml_util_freeTensorInfo(input_tensor);
    if (ret)
    {
        printf("aml_util_freeTensorInfo fail.\n");
        return -1;
    }
    ret = aml_util_freeTensorInfo(output_tensor);
    if (ret)
    {
        printf("aml_util_freeTensorInfo fail.\n");
        return -1;
    }


    ret = aml_module_destroy(qcontext);
    if (ret)
    {
        printf("aml_module_destroy fail.\n");
        return -1;
    }

    return ret;
}
