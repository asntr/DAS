import numpy as np


def get_mask(mask):
    r = mask[:, :, 0]
    g = mask[:, :, 1]
    return r // (r.max() or 1) * -1 + g // (g.max() or 1)


def calculate_brightness(image):
    weights = np.array([0.299, 0.587, 0.114])
    brightness_matrix = (image*weights).sum(axis=2)
    return brightness_matrix


def calculate_energy(brightness):
    x_gradient = np.hstack((
        (brightness[:, 1] - brightness[:, 0])[:, np.newaxis],
        brightness[:, 2:] - brightness[:, :-2],
        (brightness[:, -1] - brightness[:, -2])[:, np.newaxis]
    ))
    y_gradient = np.vstack((
        brightness[1, :] - brightness[0, :],
        brightness[2:, :] - brightness[:-2, :],
        brightness[-1, :] - brightness[-2, :]
    ))
    return np.sqrt(x_gradient ** 2 + y_gradient ** 2)


def calculate_minimal_seam_matrix(pre_energy, mask=None):
    min_seam_searcher = pre_energy + mask if mask is not None else pre_energy.copy()
    for i in range(1, min_seam_searcher.shape[0]):
        row = min_seam_searcher[i-1]
        minimum = np.vstack((np.insert(row[:-1], 0, row[0]), row, np.append(row[1:], row[-1]))).min(axis=0)
        min_seam_searcher[i] += minimum
    return min_seam_searcher


def get_minimal_seam(min_seam):
    seam = np.zeros(min_seam.shape[0], dtype=np.int32)
    seam[-1] = np.argmin(min_seam[-1])
    for i in range(min_seam.shape[0] - 2, -1, -1):
        last = seam[i+1]
        if last == 0:
            seam[i] = np.argmin(min_seam[i, : 2])
        elif last == min_seam.shape[1] - 1:
            seam[i] = last + np.argmin(min_seam[i, (last - 1):]) - 1
        else:
            seam[i] = last + np.argmin(min_seam[i, (last - 1): (last + 2)]) - 1
    return seam


def cut(image, mask):
    brightness = calculate_brightness(image)
    energy = calculate_energy(brightness)
    mult = image.shape[0] * image.shape[1] * 256
    min_seam = calculate_minimal_seam_matrix(energy, mask * mult if mask is not None else None)
    seam = get_minimal_seam(min_seam)
    copy = np.empty((image.shape[0], image.shape[1] - 1, 3), np.uint8)
    copy_mask = np.empty((image.shape[0], image.shape[1] - 1), np.int32) if mask is not None else None
    seam_mask = np.zeros(image.shape[:2], dtype=np.uint8)
    for row, i in enumerate(seam):
        copy[row] = np.delete(image[row], i, axis=0)
        if mask is not None:
            copy_mask[row] = np.delete(mask[row], i, axis=0)
        seam_mask[row][i] = 1
    return copy, copy_mask, seam_mask


def extend(image, mask):
    brightness = calculate_brightness(image)
    energy = calculate_energy(brightness)
    mult = image.shape[0] * image.shape[1] * 256
    min_seam = calculate_minimal_seam_matrix(energy, mask * mult if mask is not None else None)
    seam = get_minimal_seam(min_seam)
    copy = np.empty((image.shape[0], image.shape[1] + 1, 3), np.uint8)
    copy_mask = np.zeros((image.shape[0], image.shape[1] + 1), np.int32) if mask is not None else None
    seam_mask = np.zeros(image.shape[:2], dtype=np.uint8)
    for row, i in enumerate(seam):
        if i >= image.shape[1] - 1:
            copy[row] = np.concatenate((image[row], [image[row][-1]]), axis=0)
            if mask is not None:
                copy_mask[row] = np.append(mask[row], 0)
                copy_mask[row][-2] = 1
                copy_mask[row][-1] = 1
        else:
            copy[row] = np.insert(image[row], i+1, image[row][i] // 2 + image[row][i+1] // 2, axis=0)
            if mask is not None:
                copy_mask[row] = np.insert(mask[row], i+1, 0, axis=0)
                copy_mask[row][i] = 1
                copy_mask[row][i+1] = 1
        seam_mask[row][i] = 1
    return copy, copy_mask, seam_mask


def seam_carve(image, mode, mask):
    if mode == 'horizontal shrink':
        return cut(image, mask)
    elif mode == 'vertical shrink':
        transposed_image, transposed_mask, transposed_seam_mask = cut(
            np.transpose(image, (1, 0, 2)), mask.T if mask is not None else None
        )
        return (np.transpose(transposed_image, (1, 0, 2)),
                transposed_mask.T if mask is not None else None,
                transposed_seam_mask.T)
    elif mode == 'horizontal expand':
        return extend(image, mask)
    else:
        transposed_image, transposed_mask, transposed_seam_mask = extend(
            np.transpose(image, (1, 0, 2)), mask.T if mask is not None else None
        )
        return (np.transpose(transposed_image, (1, 0, 2)),
                transposed_mask.T if mask is not None else None,
                transposed_seam_mask.T)
