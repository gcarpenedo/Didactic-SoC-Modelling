# VP reconstruction quick reference

Scope: behavioral VP of the Didactic SoC. This note keeps only facts that affect the model and the evidence behind them. RTL paths are relative to the repository root.

## Architecture and address map

`src/generated/SysCtrl_SS_0.v` connects Ibex, JTAG debug, IMEM/DMEM SRAM, controller registers, and standard peripherals. The main OBI crossbar (`src/rtl/sysctrl_obi_xbar.sv`) has Ibex instruction/data and debug initiators, six 64 KiB targets, `NumMaxTrans=1`, and no pipeline cuts. IMEM, DMEM, debug, and controller are direct targets; peripherals use OBI-to-APB bridges. Student peripherals go through `obi_icn_ss` in `src/generated/Didactic.v` and `src/rtl/obi_icn_ss.sv`.

| Target | Base | Decode size | Model notes |
| --- | ---: | ---: | --- |
| IMEM | `0x01000000` | 64 KiB | 16 KiB SRAM; linker uses 4 KiB |
| DMEM | `0x01010000` | 64 KiB | 16 KiB SRAM; linker uses 4 KiB; stack at offset `0xC00` |
| Debug | `0x01020000` | 64 KiB | End-of-test status word `0x01020380` |
| Peripherals | `0x01030000` | 64 KiB | GPIO `+0x000`, UART `+0x100`, SPI `+0x200` |
| Controller | `0x01040000` | 64 KiB | Boot register `0x01040180` |
| Student windows | `0x01050000` | five 4 KiB windows | Analog `+0x0000`, KTH `+0x1000`, DTU `+0x2000`, TUM `+0x3000`, IMT `+0x4000` |

Use the upstream DTU window `0x01052000`-`0x01052fff`; the alternate DTU fork map starting at `0x01060000` is not used. Target 2/APB_2 wires to `i_dtu_ss_wrapper`. IMT, KTH, and TUM are simulation tie-offs.

## Core, reset, and completion

Ibex wrapper settings (`src/rtl/ibex_wrapper.sv`): `RV32MFast`, `RV32BNone`, `RV32E=0`, FF register file, no writeback stage or I-cache, `PMPEnable=0`, `SecureIbex=0`, `MHPMCounterNum=0`. `fetch_enable_i` comes from controller `fetch_en`, reset to `0x5` (`IbexMuBiOn=4'b0101`). `mcycle` and `minstret` remain present when `MHPMCounterNum=0`; optional HPM counters are disabled. Ibex revision: `8a3d46f095db817fafdad9de4d157ccb83a267fb`.

Testbench (`src/tb/tb_didactic.sv`) holds reset for 3 ms. It then loads `build/sw/<test>.hex` over JTAG at `0x01000000`, sets `dpc=0x01000080`, and resumes the core. `crt0.S` sets the stack and calls `main`; it does not clear `.bss`. On return, `postMain` writes `a0 | 0x80000000` to `0x01020380`; JTAG status zero means success. `postMain` falls through to `changeStack`, so a later illegal-instruction message can follow a successful status. Stop the VP at the status write.

## Measured behavior and model constraints

| Question | Result to use | Evidence |
| --- | --- | --- |
| O1: OBI/APB latency | IMEM `0x01000080`, DMEM `0x01010bfc`, and controller `0x01040028`: grant in request cycle, response next cycle. UART APB write `0x01030108`: grant one cycle after core request, response one cycle after grant. Student APB write `0x01052c00`: APB select with OBI request; APB ready/grant two cycles later, response next cycle. | `sim/vcd/hello.vcd`, `sim/vcd/dtutest.vcd`; 10 ns clock. |
| O2: instruction/data collision at IMEM | In this capture instruction fetches `0x94`, `0x98` were accepted before data load `0x80`; load grant came four cycles after request; fetch `0x9c` waited through response bubble. Serialize through one SRAM. Do not infer permanent fixed priority from this one collision. | `sim/vcd/o2_imem.vcd`, 7.822825-7.822885 ms. |
| O3: consecutive SRAM requests | `sp_sram` drops grant for one cycle after accepting a request while `rvalid` is asserted; repeated IMEM acceptance edges are 20 ns apart. | `src/rtl/sp_sram.sv`, `sim/vcd/hello.vcd` at 8.757025 and 8.757045 ms. |
| O4: uninitialized RAM | IMEM and DMEM word zero read `00000000`; `sp_sram.sv` initializes the RAM to zero when `INIT_FILE` is empty. This does not initialize software `.bss`. | Questa observation; `src/rtl/sp_sram.sv`, `src/generated/SysCtrl_SS_0.v`, `sw/common/crt0.S`. |
| O5: UART and completion | `hello from didactic!` is in `sim/stdout/uart`, not the Transcript. A post-return illegal instruction does not negate a preceding JTAG status of zero. | User's `hello` run; `tb_didactic.sv`, `sw/common/crt0.S`. |
| O6/O7: CSR controls | `fetch_en=0x5` is Ibex `IbexMuBiOn`. `mcountinhibit` resets to zero; `mcycle` and `minstret` are instantiated regardless of optional HPM count. | `vendor_ips/ibex/rtl/ibex_pkg.sv`, `ibex_cs_registers.sv`, `src/rtl/SS_Ctrl_reg_array.sv`. |
| O8: CSR compiler flag | GCC 16.1 rejects CSR asm with `-march=rv32imc`; it accepts `-march=rv32imc_zicsr`. | MSYS2 UCRT64 compiler; `sw/Makefile`. |
| O9: exact DTU base | Confirmed. At 7.939765 ms: core request to `0x01052000`, write data `0xA55A1234`, APB_2 selected. At 7.939775 ms APB access; at 7.939785 ms APB_2 ready and core grant; at 7.939795 ms core response. APB_2 carries `PADDR=0x01052000`, `PWRITE=1`, `PWDATA=0xA55A1234`. JTAG returns status zero. | `sim/vcd/dtu_base_test.vcd`, `build/logs/sim/2026-11-05_18-18_sim.log`, `sw/dtu_base_test/dtu_base_test.c`. |

Other model-relevant details: simulation clock is 10 ns. UART receiver output is `sim/stdout/uart`; UART divisor is 27 at 100 MHz (about 231 kbaud). Standard `.bss` is not cleared by `crt0.S`. The Ibex simulation tracer writes `sim/trace_core*.log`.
