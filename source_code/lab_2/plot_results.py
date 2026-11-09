#!/usr/bin/env python3

import csv
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

DATA_DIR = Path("data")
PLOTS_DIR = Path("plots")
ALGORITHMS = {
    "standard": ("Стандартный алгоритм", "o"),
    "winograd": ("Алгоритм Винограда", "s"),
    "winograd_optimized": ("Оптимизированный алгоритм Винограда", "^"),
}


def read_results(prefix: str, algorithm: str) -> tuple[list[int], list[float]]:
    path = DATA_DIR / f"{prefix}{algorithm}.csv"
    if not path.is_file():
        raise FileNotFoundError(f"Файл результатов не найден: {path}")

    sizes: list[int] = []
    times: list[float] = []
    with path.open(newline="", encoding="utf-8") as file:
        for row in csv.DictReader(file):
            sizes.append(int(row["size"]))
            times.append(float(row["time_ms"]))
    if not sizes:
        raise ValueError(f"Файл результатов пуст: {path}")
    return sizes, times


def create_plot(prefix: str, filename: str, title: str) -> None:
    PLOTS_DIR.mkdir(parents=True, exist_ok=True)
    figure, axes = plt.subplots(figsize=(10, 6))

    for algorithm, (label, marker) in ALGORITHMS.items():
        sizes, times = read_results(prefix, algorithm)
        axes.plot(
            sizes,
            times,
            label=label,
            marker=marker,
            linewidth=1.5,
            markersize=5,
        )

    axes.set_title(title)
    axes.set_xlabel("Размер матрицы")
    axes.set_ylabel("Среднее процессорное время, мс")
    axes.grid(True, which="major", linestyle="--", linewidth=0.7, alpha=0.7)
    axes.minorticks_on()
    axes.grid(
        True, which="minor", linestyle=":", linewidth=0.5, alpha=0.4
    )
    axes.legend(frameon=True, fancybox=False, framealpha=1, edgecolor="black")
    figure.tight_layout()
    figure.savefig(PLOTS_DIR / filename, format="svg")
    plt.close(figure)


def main() -> None:
    create_plot("even_", "even.svg", "Производительность для чётных размеров")
    create_plot("odd_", "odd.svg", "Производительность для нечётных размеров")


if __name__ == "__main__":
    main()
