/*
 * Project 2: Performance Optimization
 */

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
#include <x86intrin.h>
#endif

#include <math.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "calc_depth_naive.h"
#include "calc_depth_optimized.h"
#include "utils.h"


bool outer_in_bounds(int x, int y, int image_width, int image_height, int feature_width, int feature_height) {
    return !(y < feature_height || y >= image_height - feature_height
                    || x < feature_width || x >= image_width - feature_width);
}

bool inner_in_bounds(int x, int y, int dx, int dy, int image_width, int image_height, int feature_width, int feature_height) {
    return !(y + dy - feature_height < 0
                            || y + dy + feature_height >= image_height
                            || x + dx - feature_width < 0
                            || x + dx + feature_width >= image_width);
}

void calc_depth_optimized(float *depth, float *left, float *right,
        int image_width, int image_height, int feature_width,
        int feature_height, int maximum_displacement) {
    // Naive implementation
    for (int y = 0; y < image_height; y++) {
        for (int x = 0; x < image_width; x++) {
            if (!outer_in_bounds(x,y, image_width, image_height, feature_width, feature_height)) {
                depth[y * image_width + x] = 0;
                continue;
            }
            float min_diff = -1;
            int min_dy = 0;
            int min_dx = 0;
            for (int dy = -maximum_displacement; dy <= maximum_displacement; dy++) {
                for (int dx = -maximum_displacement; dx <= maximum_displacement; dx++) {
                    if (!inner_in_bounds(x,y,dx,dy,image_width,image_height,feature_width,feature_height)) {
                        continue;
                    }


                    float squared_diff = 0;
                    for (int box_y = -feature_height; box_y <= feature_height; box_y++) {
                        for (int box_x = -feature_width; box_x < (feature_width/4)*4; box_x+=4) {
                            int left_x = x + box_x;
                            int left_y = y + box_y;
                            int right_x = x + dx + box_x;
                            int right_y = y + dy + box_y;

                            float result[4];
                            __m128i result_vector = __mm_set1_ps(0);

                            result_vector = __mm_sub_ps(__mm_loadu_si128((__m128*)(left + left_y * image_width + left_x)), __mm_loadu_si128((__m128*)(right_y * image_width + right_x)));
                            __mm_slli_ps(result_vector, result_vector, 1);
                            __mm_srli_ps(result_vector, result_vector, 1);
                            __mmstoreu_si128((__m128i*)result, result_vector);
                            squared_diff += square_euclidean_distance(
                                    left[left_y * image_width + left_x],
                                    right[right_y * image_width + right_x]
                                    );
                        }
                        for (int box_x = (feature_width/4)*4; box_x <= feature_width; box_x++) {

                        }
                    }


                    if (min_diff == -1 || min_diff > squared_diff
                            || (min_diff == squared_diff
                                && displacement_naive(dx, dy) < displacement_naive(min_dx, min_dy))) {
                        min_diff = squared_diff;
                        min_dx = dx;
                        min_dy = dy;
                    }
                }
            }
            if (min_diff != -1) {
                if (maximum_displacement == 0) {
                    depth[y * image_width + x] = 0;
                } else {
                    depth[y * image_width + x] = displacement_naive(min_dx, min_dy);
                }
            } else {
                depth[y * image_width + x] = 0;
            }
        }
    }
}
