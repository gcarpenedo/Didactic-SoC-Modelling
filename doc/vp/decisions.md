# VP decisions and pinned environment

## Reconstruction baseline - 2026-11-05

### Toolchain and RTL revisions

- Questa Altera Starter FPGA Edition 2025.2 (simulator build 2025.05); Bender 0.32.1.
- MSYS2 UCRT64 `riscv64-unknown-elf-gcc` 16.1.0. Default flags in `sw/Makefile`: `-march=rv32imc -mabi=ilp32`; add `_zicsr` for CSR instructions.
- Ibex: `8a3d46f095db817fafdad9de4d157ccb83a267fb` (`Bender.yml`). Wrapper: `RV32MFast`, `RV32BNone`, `RV32E=0`, FF regfile, no writeback stage/I-cache/PMP, `SecureIbex=0`, `MHPMCounterNum=0`; debug base `0x01020000`, boot `0x01040100`, fetch enable reset `0x5`.
- Root `Bender.lock` is ignored/absent. Commits below are the currently resolved `.bender/git/checkouts`; project constraints are in `Bender.yml`.

| Dependency | Commit |
| --- | --- |
| `apb` | `77ddf073f194d44b9119949d2421be59789e69ae` |
| `apb_gpio` | `f82caeb7f7d89427f05e9af5ed31e0675efe0d83` |
| `apb_uart` | `6c7dde3d749ac8274377745c105da8c8b8cd27c6` |
| `axi` | `2f395b176bee1c769c80f060a4345fda965bb04b` |
| `common_cells` | `1281545696eb3fcba50ec5b4275993476a3c710e` |
| `common_verification` | `fb1885f48ea46164a10568aeff51884389f67ae3` |
| `obi` | `ad1d48f025be540344960ea83b4bff39876f9b36` |
| `register_interface` | `146501d80052b61475cdc333d3aab4cd769fd5dc` |
| `riscv-dbg` | `618ee6e0e2610ef47e0dcc4df6748af3dffff731` |
| `tech_cells_generic` | `3a3de73632a06826b1bd9c65a0a2e92b32016845` |
| `vbench` | `ced0f7afe0cfb6a3588199b362859685d1902576` |

### Modeling decisions

- Use the upstream map documented in `recon.md`; DTU is `0x01052000`-`0x01052fff`, not the alternate fork map beginning at `0x01060000`.
- Preserve observed OBI/APB handshakes and the one-cycle SRAM grant bubble. The O2 trace shows one contention sequence only; it does not justify a global fixed-priority rule.
- Keep `mcycle` and `minstret` in the model with `MHPMCounterNum=0`; that value disables optional HPM counters only.
- Base memory starts at zero in this simulation, while firmware does not clear `.bss`; model both facts separately.
- Confirmed DTU base write and successful JTAG status are recorded in `sim/vcd/dtu_base_test.vcd` and `recon.md`.
