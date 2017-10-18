from skimage.transform import rescale
from sklearn.metrics import mean_squared_error
from itertools import product
import os
import numpy as np
import skimage.io as skio


def divide_image(image, divisions=3):
    n, m = image.shape
    step = n // divisions
    width_margin = m // 100 * 5
    height_margin = n // 300 * 5
    for i in range(divisions):
        yield image[i*step + height_margin: (i+1)*step - height_margin, width_margin:-width_margin]


def rectangle_intersection(starts, finishes):
    start = (np.max([x[0] for x in starts]), np.max([y[1] for y in starts]))
    finish = (np.min([x[0] for x in finishes]), np.min([y[1] for y in finishes]))
    return start, finish


def rescale_channel(image):
    return image[::2, ::2]


def mse(i1, i2):
    assert i1.shape == i2.shape
    return ((i1 - i2) ** 2).mean(axis=None)


def cross_corr(i1, i2):
    assert i1.shape == i2.shape
    return (i1 * i2).sum() / np.sqrt((i1 ** 2).sum() * (i2 ** 2).sum())


def shift(i1, i2, dx, dy):
    i1_shape, i2_shape = i1.shape, i2.shape
    start, finish = rectangle_intersection(((0, 0), (dx, dy)), (i1_shape, (dx + i2_shape[0], dy + i2_shape[1])))
    return i1[start[0]:finish[0], start[1]:finish[1]], i2[start[0] - dx:finish[0] - dx, start[1] - dy:finish[1] - dy]


def find_best_shift_for_two(i1, i2, dx_range, dy_range):
    best_shift = (0, 0)
    minimal_mse = np.infty
    #     cc = 0
    for dx, dy in product(range(*dx_range), range(*dy_range)):
        i1_shifted, i2_shifted = shift(i1, i2, dx, dy)
        cur_shift_mse = mse(i1_shifted, i2_shifted)
        #         cur_shift_cc = cross_corr(i1_shifted, i2_shifted)
        #         if cc < cur_shift_cc:
        if minimal_mse > cur_shift_mse:
            #             cc = cur_shift_cc
            minimal_mse = cur_shift_mse
            best_shift = (dx, dy)

    return best_shift


def find_best_shift_for_three(r, g, b, r_dx_range=(-15, 16), r_dy_range=(-15, 16), b_dx_range=(-15, 16),
                              b_dy_range=(-15, 16)):
    shape = r.shape
    best_shift_for_red = find_best_shift_for_two(g, r, r_dx_range, r_dy_range)
    best_shift_for_blue = find_best_shift_for_two(g, b, b_dx_range, b_dy_range)
    start, finish = rectangle_intersection(
        starts=(
            (0, 0), best_shift_for_red, best_shift_for_blue
        ),
        finishes=(
            shape,
            (best_shift_for_red[0] + shape[0], best_shift_for_red[1] + shape[1]),
            (best_shift_for_blue[0] + shape[0], best_shift_for_blue[1] + shape[1]),
        )
    )
    return (
        np.dstack((
            r[start[0] - best_shift_for_red[0]:finish[0] - best_shift_for_red[0],
            start[1] - best_shift_for_red[1]:finish[1] - best_shift_for_red[1]],
            g[start[0]:finish[0], start[1]:finish[1]],
            b[start[0] - best_shift_for_blue[0]:finish[0] - best_shift_for_blue[0],
            start[1] - best_shift_for_blue[1]:finish[1] - best_shift_for_blue[1]],
        )),
        best_shift_for_red,
        best_shift_for_blue,
    )


def pyramid_method(r, g, b):
    if r.shape[0] > 500 or r.shape[1] > 500:
        _, r_shift, b_shift = pyramid_method(rescale_channel(r), rescale_channel(g), rescale_channel(b))
        return find_best_shift_for_three(
            r, g, b,
            r_dx_range=(r_shift[0] * 2 - 2, r_shift[0] * 2 + 2),
            r_dy_range=(r_shift[1] * 2 - 2, r_shift[1] * 2 + 2),
            b_dx_range=(b_shift[0] * 2 - 2, b_shift[0] * 2 + 2),
            b_dy_range=(b_shift[1] * 2 - 2, b_shift[1] * 2 + 2),
        )
    return find_best_shift_for_three(r, g, b)


def align(image, g_coord):
    b, g, r = tuple(divide_image(image))
    width_margin = image.shape[1] // 100 * 5
    height_margin = image.shape[0] // 300 * 5
    relative_coord = (g_coord[0] - b.shape[0] - 3 * height_margin, g_coord[1] - width_margin)
    colored, r_shift, b_shift = pyramid_method(r, g, b)
    red = (relative_coord[0] - r_shift[0] + b.shape[0] * 2 + 5 * height_margin, relative_coord[1] - r_shift[1] + width_margin)
    blue = (relative_coord[0] + height_margin - b_shift[0], relative_coord[1] - b_shift[1] + width_margin)
    return colored, blue, red
