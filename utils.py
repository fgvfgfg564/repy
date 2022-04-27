import numpy as np


def checkDimension(a: np.array, n: int):
    if len(a.shape) != n:
        raise ValueError(
            f"Dimension mismatch! Expected: {n}; Found: {len(a.shape)} (shape={a.shape})"
        )


def checkDimensionMatch(a: np.array, b: np.array, n: int):
    if a.shape[n] != b.shape[n]:
        raise ValueError(
            f"Tensors have a shape mismatch at dimension {n}: {a.shape[n]} vs. {b.shape[n]}"
        )
