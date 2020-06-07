#include "ui_pick.h"
#include "ui_graphics.h"
#include <assert.h>
#include <stdio.h>

#define NULL_CODE 0xffffffff


struct Segment {
    int offset, size; // in bits
    unsigned mask;
};

inline unsigned _make_mask(int bits) {
    unsigned mask = 0;
    for (int i = 0; i < bits; ++i)
        mask |= (1 << i);
    return mask;
}

inline unsigned _rgba_to_code(unsigned char *rgba) {
    return (rgba[0] << 24) | (rgba[1] << 16) | (rgba[2] << 8) | rgba[3];
}

inline vec4 _code_to_vec4(unsigned v) {
    return vec4(((v >> 24) & 0xff) / 255.0, ((v >> 16) & 0xff) / 255.0, ((v >> 8) & 0xff) / 255.0, (v & 0xff) / 255.0);
}

inline unsigned _embed_segment(unsigned v, Segment seg) {
    return (v & seg.mask) << seg.offset;
}

inline unsigned _extract_segment(unsigned code, Segment seg) {
    return (code >> seg.offset) & seg.mask;
}

inline unsigned _code_to_category(unsigned v) {
    return v & CATEGORY_MASK;
}

inline float _denormalize_depth(float z, float near, float far) {
    return near * far / (far - z * (far - near)) - near;
}

struct Category {
    int segs_count;
    Segment segs[MAX_SEGMENTS];

    Category() {
        define_encoding();
    }

    void define_encoding() {
        segs_count = 1;
        segs[0].offset = CATEGORY_OFFSET;
        segs[0].size = MAX_BITS - segs[0].offset;
        segs[0].mask = _make_mask(segs[0].size);
    }

    void define_encoding(int size1) {
        assert(CATEGORY_OFFSET + size1 < MAX_BITS);
        segs_count = 2;
        segs[0].offset = CATEGORY_OFFSET;
        segs[0].size = size1;
        segs[0].mask = _make_mask(segs[0].size);
        segs[1].offset = CATEGORY_OFFSET + size1;
        segs[1].size = MAX_BITS - segs[1].offset;
        segs[1].mask = _make_mask(segs[1].size);
    }

    void define_encoding(int size1, int size2) {
        assert(CATEGORY_OFFSET + size1 + size2 < MAX_BITS);
        segs_count = 3;
        segs[0].offset = CATEGORY_OFFSET;
        segs[0].size = size1;
        segs[0].mask = _make_mask(segs[0].size);
        segs[1].offset = CATEGORY_OFFSET + size1;
        segs[1].size = size2;
        segs[1].mask = _make_mask(segs[1].size);
        segs[2].offset = CATEGORY_OFFSET + size1 + size2;
        segs[2].size = MAX_BITS - segs[2].offset;
        segs[2].mask = _make_mask(segs[2].size);
    }

    vec4 encode(unsigned category_id, unsigned v0) {
        assert(segs_count == 1);
        return _code_to_vec4(category_id | _embed_segment(v0, segs[0]));
    }

    vec4 encode(unsigned category_id, unsigned v0, unsigned v1) {
        assert(segs_count == 2);
        return _code_to_vec4(category_id | _embed_segment(v0, segs[0]) | _embed_segment(v1, segs[1]));
    }

    vec4 encode(unsigned category_id, unsigned v0, unsigned v1, unsigned v2) {
        assert(segs_count == 3);
        return _code_to_vec4(category_id | _embed_segment(v0, segs[0]) | _embed_segment(v1, segs[1]) | _embed_segment(v2, segs[2]));
    }

    void decode(unsigned code, unsigned *ids) {
        for (int i = 0; i < segs_count; ++i)
            ids[i] = _extract_segment(code, segs[i]);
    }
};

unsigned category_count = 0;
Category categories[MAX_CATEGORIES];

// :public

PickResult::PickResult() : hit(false) {}

void PickResult::clear() {
    hit = false;
}

unsigned pick_register_category() {
    return category_count++;
}

void pick_define_encoding(unsigned category_id, int size1) {
    assert(category_id < category_count);
    categories[category_id].define_encoding(size1);
}

void pick_define_encoding(unsigned category_id, int size1, int size2) {
    assert(category_id < category_count);
    categories[category_id].define_encoding(size1, size2);
}

vec4 pick_encode(unsigned category_id, unsigned v0) {
    assert(category_id < category_count);
    return categories[category_id].encode(category_id, v0);
}

vec4 pick_encode(unsigned category_id, unsigned v0, unsigned v1) {
    assert(category_id < category_count);
    return categories[category_id].encode(category_id, v0, v1);
}

vec4 pick_encode(unsigned category_id, unsigned v0, unsigned v1, unsigned v2) {
    assert(category_id < category_count);
    return categories[category_id].encode(category_id, v0, v1, v2);
}

void pick(int pick_radius, int w, int h, int near, int far, PickResult &result, bool get_depth) {
    int pixels_side = pick_radius * 2;
    int left = w * 0.5 - pick_radius;
    int bottom = h * 0.5 - pick_radius;

    static unsigned char rgba[4 * 10 * 10]; // TODO: fix this array size
    graph_read_pixels(left, bottom, pixels_side, pixels_side, rgba);

    ivec2 positions[MAX_CATEGORIES];
    unsigned codes[MAX_CATEGORIES];
    unsigned layers[MAX_CATEGORIES];

    for (int i = 0; i < MAX_CATEGORIES; ++i)
        codes[i] = NULL_CODE;

    for (int r = 1; r <= pick_radius; ++r) {

        int x = pick_radius - r;
        int y = pick_radius - r;
        unsigned code, category;

        for (; x < pick_radius + r - 1; ++x) {
            code = _rgba_to_code(rgba + (y * pixels_side + x) * 4);
            if (code == NULL_CODE)
                continue;
            category = _code_to_category(code);
            if (codes[category] != NULL_CODE)
                continue;
            codes[category] = code;
            layers[category] = r;
            positions[category] = ivec2(x, y);
        }
        for (; y < pick_radius + r - 1; ++y) {
            code = _rgba_to_code(rgba + (y * pixels_side + x) * 4);
            if (code == NULL_CODE)
                continue;
            category = _code_to_category(code);
            if (codes[category] != NULL_CODE)
                continue;
            codes[category] = code;
            layers[category] = r;
            positions[category] = ivec2(x, y);
        }
        for (; x > pick_radius - r; --x) {
            code = _rgba_to_code(rgba + (y * pixels_side + x) * 4);
            if (code == NULL_CODE)
                continue;
            category = _code_to_category(code);
            if (codes[category] != NULL_CODE)
                continue;
            codes[category] = code;
            layers[category] = r;
            positions[category] = ivec2(x, y);
        }
        for (; y > pick_radius - r; --y) {
            code = _rgba_to_code(rgba + (y * pixels_side + x) * 4);
            if (code == NULL_CODE)
                continue;
            category = _code_to_category(code);
            if (codes[category] != NULL_CODE)
                continue;
            codes[category] = code;
            layers[category] = r;
            positions[category] = ivec2(x, y);
        }
    }

    result.clear();

    for (int i = 0; i < category_count; ++i) {
        if (codes[i] != NULL_CODE) {
            result.hit = true;
            result.code = codes[i];
            result.category_id = i;
            break;
        }
    }

    if (result.hit) {
        categories[result.category_id].decode(result.code, result.ids);

        if (get_depth) {
            int x = left + positions[result.category_id].x;
            int y = bottom + positions[result.category_id].y;

            graph_read_depth(x, y, 1, 1, &result.depth);
            result.depth = _denormalize_depth(result.depth, near, far);
        }
    }
}
