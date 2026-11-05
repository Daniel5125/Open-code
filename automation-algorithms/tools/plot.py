#!/usr/bin/env python3
"""Genera las figuras de la guía a partir de los ejemplos en C.

    cmake -S . -B build && cmake --build build
    python tools/plot.py build docs/img
"""
import csv
import subprocess
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

plt.rcParams.update({"figure.dpi": 130, "axes.grid": True, "grid.alpha": 0.3, "axes.spines.top": False,
                     "axes.spines.right": False, "font.size": 10})
C = ["#0f766e", "#d97706", "#2563eb", "#be185d"]


def run(build: Path, name: str) -> list[dict]:
    out = subprocess.run([str(build / name)], check=True, capture_output=True, text=True).stdout
    return list(csv.DictReader(out.splitlines()))


def col(rows, k):
    return [float(r[k]) for r in rows]


def pid(build, out):
    r = run(build, "pid_windup")
    t = col(r, "t")
    fig, (a, b) = plt.subplots(2, 1, figsize=(8, 5.5), sharex=True, gridspec_kw={"height_ratios": [3, 2]})
    a.plot(t, col(r, "consigna"), "k--", lw=1, label="consigna")
    a.plot(t, col(r, "y_sin"), color=C[3], label="PI sin anti-windup")
    a.plot(t, col(r, "y_cond"), color=C[0], label="integración condicional")
    a.plot(t, col(r, "y_retro"), color=C[2], ls="-.", label="retrocálculo")
    a.set_ylabel("temperatura [°C]")
    a.legend(loc="upper right", fontsize=8)
    a.set_title("Windup: la consigna de 230 °C es inalcanzable (máx. 200 °C) y luego baja a 120 °C")
    b.plot(t, col(r, "u_sin"), color=C[3])
    b.plot(t, col(r, "u_cond"), color=C[0])
    b.plot(t, col(r, "u_retro"), color=C[2], ls="-.")
    b.set_ylabel("calefactor [%]")
    b.set_xlabel("tiempo [s]")
    fig.tight_layout()
    fig.savefig(out / "pid_windup.png")


def ramp(build, out):
    r = run(build, "stepper_profile")
    fig, ax = plt.subplots(figsize=(8, 3.6))
    for i, name in enumerate(["largo_3000", "corto_300"]):
        rr = [x for x in r if x["movimiento"] == name]
        ax.plot(col(rr, "t_s"), col(rr, "velocidad_pasos_s"), color=C[i],
                label={"largo_3000": "3000 pasos (trapecio)", "corto_300": "300 pasos (triángulo)"}[name])
    ax.axhline(1000, color="k", ls="--", lw=0.8)
    ax.text(0.02, 1015, "velocidad máxima", fontsize=8)
    ax.set_xlabel("tiempo [s]")
    ax.set_ylabel("velocidad [pasos/s]")
    ax.set_title("Perfil de velocidad calculado paso a paso (a = 2000 pasos/s²)")
    ax.legend(fontsize=8)
    fig.tight_layout()
    fig.savefig(out / "stepper_profile.png")


def debounce(build, out):
    r = run(build, "debounce_demo")
    t = col(r, "t_ms")
    fig, ax = plt.subplots(figsize=(8, 2.8))
    ax.step(t, [v + 1.3 for v in col(r, "crudo")], where="post", color=C[3], label="entrada cruda")
    ax.step(t, col(r, "filtrado"), where="post", color=C[0], lw=2, label="salida filtrada")
    ax.set_yticks([0, 1, 1.3, 2.3], ["0", "1", "0", "1"])
    ax.set_xlabel("tiempo [ms] (muestreo a 1 kHz, 10 muestras)")
    ax.annotate("rebotes", (55, 2.35), fontsize=8)
    ax.annotate("interferencia de 4 ms\n(ignorada)", (125, 1.5), fontsize=8)
    ax.legend(loc="center right", fontsize=8)
    fig.tight_layout()
    fig.savefig(out / "debounce.png")


if __name__ == "__main__":
    build, out = Path(sys.argv[1]), Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    pid(build, out)
    ramp(build, out)
    debounce(build, out)
    print("figuras en", out)
