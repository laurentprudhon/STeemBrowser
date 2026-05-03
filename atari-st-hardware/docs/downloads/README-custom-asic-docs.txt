# Atari ST Custom ASICs - Comprehensive Documentation Index

> No official datasheets were ever published for Atari's custom GST (Glue Semiconductor Technology) chips. The information below is compiled from the most comprehensive reverse-engineering efforts available online.

## ASIC Summary

| ASIC | Part Numbers | Era | Notes |
|------|-------------|-----|-------|
| GLUE | C029144, C300866, C301578 | STF/STe | Bus glue, refresh, address decoding |
| MMU | C028300-2, C30114 | STF/STe | DRAM controller, scroll engine |
| DMA | C029128-1, C30144 | STF/STe | Floppy/HDC DMA arbiter |
| SHIFTER | C028787-2, C028761-1 | STF/STe | Video D/A, sync, audio mixer |
| GST MCU | C302183 | STe/MegaSTE | GLUE+MMU combined (STe) |
| GST SHIFTER | C029145 | STe/MegaSTE | Enhanced video (Super Shifter) |
| GST DMA | C301842 | STe/MegaSTE | Enhanced DMA (STe) |
| GST BLITTER | C301842 | STe/MegaSTE | 16-bit graphics co-processor |

**Note**: GST stands for "Glue Semiconductor Technology", Atari's custom ASIC division. Atari also operated a division called Styra Semiconductor Corporation which was later purchased by Atari Corp.

---

## 1. GLUE Chip (C029144 / C300866 / C301578)

### Available Documentation

1. **[Atari ST Internals - Ch. 1.5: The Glue Chip PDF](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)** (also at `atari_st_internals.pdf` in this directory)
   - Author: Klaus Heidenreich / Abacus
   - Original schematics from the ST chipset
   - 10.9 MB comprehensive document covering GLUE, MMU, DMA, SHIFTER

2. **[Anatomy of the Atari ST PDF](https://www.atarimania.com/documents/AnatomyOfTheAtariSt.pdf)** (also at `anatomy_atari_st.pdf`)
   - Ultra-detailed (18.7 MB) transistor-level analysis
   - Complete signal flow diagrams, timing analysis
   - Board-level schematics with transistor descriptions

3. **[CHZ-Soft: Recovering Atari ST ASIC designs](https://www.chzsoft.de/asic-web/)** (also at `custom_silicon_chzsoft.html`)
   - Christian Zietz's original discovery article (Aug 2016)
   - Recovered schematics from Atari HQ backup archives
   - PDF schematics at gate/transistor level for GLUE, MMU, DMA, SHIFTER
   - Historical context: data found on FastBack backup floppies from Atari HQ in Texas

4. **[Keli.dk: Secret GAMECART Register - Spelunking in Atari STE Silicon](https://www.keli.dk/old-asic/)** (also at `custom_silicon_keli.html`)
   - Keli Hlodversson's deep-dive into GST MCU silicon
   - Discovery of undocumented GAMECART register at $FF9000
   - Analysis of GLUE/MMU internal signals and undocumented functionality
   - RAM expansion hacks through CPU pins

5. **[Hackaday: Detective Work Recovers Atari ST ASIC Designs](https://hackaday.com/2018/09/05/detective-work-recovers-atari-st-asic-designs/)** (also at `custom_silicon_hackaday.html`)
   - Overview article on the digital archaeology effort
   - Background on how Christian Zietz recovered the schematics

6. **[Atari ST System-on-Chip in VHDL](https://devlynx.ti-fr.com/ST/dev-docs.atariforge.org/alltogether.pdf)** (also at `vhdl_system_on_chip.pdf`)
   - Lyndon Amsdon's university project (Ch. 5 - Implementation)
   - GLUE IP core implementation details (Ch. 5.12)
   - MMU IP core (Ch. 5.13)
   - Transaction-level analysis of all four custom chips

7. **[STe Hardware - Info-Coach](https://info-coach.fr/atari/hardware/STE-HW.php)** (also at `custom_silicon_stee_hw.html`)
   - French ST Magazine N°44 (Sept 1990) translation
   - GST MCU = Glue + MMU combo description
   - Pin mapping and signal descriptions for STE era

8. **[Atarihacks - Atari hacking collective](https://atarimania.com/ AtariMania reference)**
   - Chris Swinson's RAM expansion hacks for Atarihacks
   - Exploitation of CPU pins 22/23 for additional DRAM signals
   - GLUE address decoding analysis

9. **[ST Internals PDF (Raw GitHub)](https://raw.githubusercontent.com/sporniket/kicad-symbols-generated/main/reference-materials/atari-16-32/st-glue-mmu-dma-shifter--Atari-ST-Internals.pdf)**
   - Raw reference materials for KiCad symbol generation
   - GLUE, MMU, DMA, SHIFTER functional descriptions

---

## 2. MMU Chip (C028300-2 / C30114)

### Available Documentation

1. **[Atari ST Internals - Ch. 1.3: DMA Controller](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)** (in `atari_st_internals.pdf`)
   - Complete MMU functional description
   - DRAM refresh cycle analysis
   - Scroll register implementation details

2. **[Anatomy of the Atari ST PDF](https://www.atarimania.com/documents/AnatomyOfTheAtariSt.pdf)** (in `anatomy_atari_st.pdf`)
   - MMU schematic diagrams with transistor-level details
   - DRAM page management analysis
   - Scroll engine timing

3. **[GST MCU (GLUE+MMU) Verilog Model - GitHub](https://github.com/gyurco/gstmcu)** (also at `gstmcu_github.html`)
   - Gyurco's cycle-accurate Verilog model of GST MCU (combined GLUE+MMU)
   - Based on recovered schematics from Christian Zietz's collection
   - The most detailed functional simulation available
   - Used in AtariST_MiSTer FPGA core

4. **[MiSTer AtariST Project - GitHub](https://github.com/MiSTer-devel/AtariST_MiSTer)** (also at `miester_atarist.html`)
   - Cycle-accurate STe GLUE+MMU combo implementation
   - Based on original schematics recovered from Atari archives
   - Verilog source code with detailed timing analysis
   - FPGA target: MiSTer platform

5. **[Atari ST System-on-Chip VHDL - Ch. 5.13: MMU IP core](https://devlynx.ti-fr.com/ST/dev-docs.atariforge.org/alltogether.pdf)** (in `vhdl_system_on_chip.pdf`)
   - Transaction-level MMU model
   - Internal register descriptions
   - DRAM refresh scheduling algorithm

6. **[Atarihacks RAM Expansion Hacks](https://atarimania.com/)** (referenced in `custom_silicon_keli.html`)
   - Exploitation of MMU address generation
   - Hacking pins 22/23 for additional DRAM bank selection
   - 4MB+ RAM on original ST

---

## 3. DMA Chip (C029128-1 / C30144)

### Available Documentation

1. **[Atari ST Internals - Ch. 1.3: DMA Controller](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)** (in `atari_st_internals.pdf`)
   - DMA interface descriptions
   - Floppy DMA handshake protocol
   - Hard disk DMA timing

2. **[Anatomy of the Atari ST PDF](https://www.atarimania.com/documents/AnatomyOfTheAtariSt.pdf)** (in `anatomy_atari_st.pdf`)
   - DMA board-level schematics
   - Signal flow diagrams
   - DMA arbitration logic

3. **[CHZ-Soft: DMA schematics](https://www.chzsoft.de/asic-web/)** (in `custom_silicon_chzsoft.html`)
   - Recovered original schematics at gate level
   - Multiple revisions of the DMA chip design

---

## 4. SHIFTER Chip (C028787 / C028761) + GST SHIFTER (C029145)

### Available Documentation for Standard ST Shifter

1. **[Atari ST Internals - Ch. 1.4: Shifter/VDC](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)** (in `atari_st_internals.pdf`)
   - Video timing analysis
   - DAC descriptions
   - Sync generation

2. **[Anatomy of the Atari ST PDF](https://www.atarimania.com/documents/AnatomyOfTheAtariSt.pdf)** (in `anatomy_atari_st.pdf`)
   - Shifter board-level schematics
   - Pixel clock analysis

3. **[CHZ-Soft: SHIFTER schematics](https://www.chzsoft.de/asic-web/)** (in `custom_silicon_chzsoft.html`)
   - Original Atari schematics in PDF format
   - Transistor-level diagrams
   - Multiple design revisions found in Atari archives

4. **[CHZ-Soft: GAME SHIFTER (ST-4118) - Unreleased Panther chip](https://www.chzsoft.de/asic-web/)** (in `custom_silicon_chzsoft.html`)
   - ST-4118 video generator ASIC (84-pin LLC)
   - Found on Panther development board
   - 32-entry 18-bit CRAM palette
   - Object processor ASIC design
   - Multiple sets of 5 registers with 16-bit inputs
   - Analysis by kool kitty89 on AtariAge (in `custom_silicon_atariage.html`)

### Available Documentation for GST Enhanced Shifter (C029145) - "Super Shifter"

1. **[STe Hardware - Info-Coach](https://info-coach.fr/atari/hardware/STE-HW.php)** (in `custom_silicon_stee_hw.html`)
   - GST Shifter as video management component
   - Sound DMA integration
   - Pin signal descriptions (active-low convention documented)

2. **[AtariAge: Atari ST ASIC designs and GAME SHIFTER](https://forums.atariage.com/topic/298373-atari-st-asic-designs-and-game-shifter/)** (in `custom_silicon_atariage.html`)
   - Comprehensive analysis of GAME SHIFTER (ST-4118) by kool kitty89
   - Comparison between standard ST Shifter and Panther/GAME SHIFTER
   - Potential STe Super Shifter vs GAME SHIFTER relationship
   - Detailed analysis of pixel pipeline, palette, and line buffer architecture
   - Notes on Panther development board timing (32.2159 MHz clock)

3. **[Keli.dk: Secret GAMECART Register](https://www.keli.dk/old-asic/)** (in `custom_silicon_keli.html`)
   - GST MCU/MMU analysis
   - Undocumented register discovery
   - Connection between GLUE/MMU and Shifter address generation

4. **[Atari ST System-on-Chip VHDL - Ch. 5.15: Shifter IP Core](https://devlynx.ti-fr.com/ST/dev-docs.atariforge.org/alltogether.pdf)** (in `vhdl_system_on_chip.pdf`)
   - Shifter IP core implementation details
   - Video timing and DAC analysis
   - Pixel engine description

---

## 5. GST MCU (C302183) - GLUE + MMU Combined (STe Era)

### Available Documentation

1. **[GST MCU Verilog Model - GitHub](https://github.com/gyurco/gstmcu)** (in `gstmcu_github.html`)
   - **Most comprehensive cycle-accurate simulation model available**
   - Verilog implementation based on recovered Atari schematics
   - Exact replica of GST MCU (combined GLUE+MMU)
   - Used in production-quality FPGA emulation

2. **[Keli.dk: Secret GAMECART Register](https://www.keli.dk/old-asic/)** (in `custom_silicon_keli.html`)
   - Deep silicon analysis of GST MCU
   - Discovery of undocumented $FF9000 register
   - Internal signal mapping within the GLUE+MMU combo

3. **[MiSTer AtariST - Cycle-accurate GLUE+MMU](https://github.com/MiSTer-devel/AtariST_MiSTer)** (in `miester_atarist.html`)
   - Cycle-accurate STe GLUE+MMU implementation
   - Based on recovered schematics via CHZ-Soft

4. **[STe Hardware - Info-Coach](https://info-coach.fr/atari/hardware/STE-HW.php)** (in `custom_silicon_stee_hw.html`)
   - GST MCU functional description
   - "Contains most of the internal Atari glue logic, DRAM Management, Clocks generation, Address decoding, and the New joysticks/paddles/pen manangement"

---

## 6. GST SHIFTER (C029145) - Enhanced Shifter (STe Era)

### Available Documentation

1. **[STe Hardware - Info-Coach](https://info-coach.fr/atari/hardware/STE-HW.php)** (in `custom_silicon_stee_hw.html`)
   - "GST Shifter (Atari): Video management component, contains also the sound DMA"
   - Enhanced color register descriptions
   - Super Hi-Res support

2. **[CHZ-Soft: Recovered GST Shifter schematics](https://www.chzsoft.de/asic-web/)** (in `custom_silicon_chzsoft.html`)
   - Original 1980s schematics restored from Atari archives
   - Transistor-level implementation

3. **[AtariAge GAME SHIFTER analysis](https://forums.atariage.com/topic/298373-atari-st-asic-designs-and-game-shifter/)** (in `custom_silicon_atariage.html`)
   - kool kitty89's analysis of video shifter architecture
   - Comparison between Panther and STe Super Shifter designs

---

## 7. GST DMA (C301842) - Enhanced DMA (STe Era)

### Available Documentation

1. **[STe Hardware - Info-Coach](https://info-coach.fr/atari/hardware/STE-HW.php)** (in `custom_silicon_stee_hw.html`)
   - Address management for 8-bit stereophonic sound DMA
   - Enhanced floppy/HDC handshake

2. **[CHZ-Soft:** (various recovered schematics in `custom_silicon_chzsoft.html`)

---

## 8. GST BLITTER (C301842) - 16-bit Graphics Co-processor (STe Era)

### Available Documentation

1. **[BLiTTER Manual](../blitter-manual.pdf)** - Existing wiki documentation
   - BLiTTER command set and data format
   - Bit-plane blitting operations
   - Memory-to-memory and screen-to-screen transfers
   - 16-bit wide data path

2. **[Anatomy of the Atari ST PDF](https://www.atarimania.com/documents/AnatomyOfTheAtariSt.pdf)** (in `anatomy_atari_st.pdf`)
   - BLiTTER block diagrams
   - Address/data bus interface

3. **[CHZ-Soft: Blitter schematics](https://www.chzsoft.de/asic-web/)** (in `custom_silicon_chzsoft.html`)
   - Original Blitter schematics recovered

---

## FPGA Emulation Reference Implementations

### Key Reverse-Engineering Projects

1. **[gstmcu - GST MCU Verilog Model](https://github.com/gyurco/gstmcu)** (in `gstmcu_github.html`)
   - Cycle-accurate GLUE+MMU simulation
   - Based on original Atari schematics

2. **[AtariST_MiSTer - MiSTer FPGA Core](https://github.com/MiSTer-devel/AtariST_MiSTer)** (in `miester_atarist.html`)
   - Cycle-accurate ST/STe FPGA implementation
   - Jorge Cwik's cycle-accurate Blitter
   - Jorge Cwik's cycle-accurate shifter (video + audio)
   - GST MCU recreated from original schematics

3. **[Hatari Emulator](https://www.hatari-emu.org/doc/manual.html)** (in `hatari_manual.html`)
   - Cycle-accurate software emulation of all custom chips
   - Source code available at `github.com/nkimiriel/hatari`
   - Used for compatibility testing of ST software

4. **[Steem SSE](https://sourceforge.net/projects/steemsse/)** (in `steem_sse_source.html`)
   - Cycle-accurate Atari ST emulator
   - Detailed shifter/MMU implementation

5. **[VHDL System-on-Chip](https://devlynx.ti-fr.com/ST/dev-docs.atariforge.org/alltogether.pdf)** (in `vhdl_system_on_chip.pdf`)
   - University project implementing all four custom chips in VHDL
   - GLUE IP core (Ch. 5.12)
   - MMU IP core (Ch. 5.13)
   - Shifter IP core (Ch. 5.15)

6. **[Building an Atari ST on FPGA - Zerkman](https://zerkman.sector1.fr/index.php?post/2020/10/09/Building-an-Atari-ST)** (in `zerkman_fpga_st.html`)
   - FPGA implementation blog series
   - Shifter Inside project (border removal demo)
   - Custom keyboard driver implementation

---

## VME Controller (Mega STE Only)

### Available Documentation

1. **[Atari VME Expansion for TT030 and Mega STE Products Spec](http://ftp.pigwa.net/stuff/collections/Atari%20documents/Manuals/Atari%20TT030/vme_spec_7-19-1991.pdf)** (in `vme_spec_tt030_megaste.pdf`)
   - 126 KB official Atari spec
   - VME C.1 slot specification
   - Bus timing and protocol

2. **[Inside the Atari Mega STe](https://www.goto10retro.com/p/inside-the-atari-mega-ste)** (in `megaste_inside_goto10.html`)
   - Paul Lefebvre's deep-dive into Mega STE board
   - VME controller board description
   - 16 MHz cache analysis

3. **[pro_VME VMEST - rare ST clone](https://randoc.wordpress.com/2024/04/11/pro_vme-vmest-the-rarest-of-the-contemporary-atari-st-clones/)**
   - Industrial clone of Mega ST for VME racks
   - Custom VME interface details

---

## Falcon Custom Chips (Bonus - For Reference)

### Available Documentation

1. **[Falcon Custom Chips Decap - AtariAge](https://forums.atariage.com/topic/382700-atari-falcon-custom-chips-decapsulated-and-available-for-reverse-engineering/)** (in `falcon_decap_atariage.html`)
2. **[Falcon Custom ICs Decap - exxos forum](https://www.exxosforum.co.uk/forum/viewtopic.php?p=133569)** (in `falcon_decap_exxos.html`)
3. **[Falcon Custom ICs Decap - Atari-Forum](https://www.atari-forum.com/viewtopic.php?t=45152)** (in `falcon_decap_atari_forum.html`)

Key Falcon chips (reverse engineering ongoing):
- **Videl** - LSI Gate Array video IC
- **Combel** - Motorola Gate Array (180 I/O pads)
- **Sdma** - Motorola Gate Array (same base as Combel)

---

## No Public Documentation Available

### Confirmed Gaps

1. **GST MCU (C302183)** - No official datasheet exists. Relies on:
   - Recovered schematics from CHZ-Soft (via Atari archives)
   - Verilog model at github.com/gyurco/gstmcu
   - ST Magazine N°44 (French, 1990) description
   - Keli.dk silicone spelunking

2. **GST Shifter (C029145)** - No official datasheet. Relies on:
   - Recovered schematics from CHZ-Soft
   - ST Magazine description
   - FPGA implementations

3. **GST DMA (C301842)** - No official datasheet

4. **GST Blitter (C301842)** - No official datasheet
   - Some information in CHZ-Soft schematics
   - Existing wiki documentation

5. **GAME SHIFTER (ST-4118)** - Unreleased Panther chip
   - Schematics available at CHZ-Soft
   - Analysis at AtariAge by kool kitty89

### How to Help

Christian Zietz's CHZ-Soft project has the original schematics recovered from Atari HQ archives. If you have access to additional archives or have found more documents, the community would welcome the contribution.

---

## Downloaded Files in This Directory

| File | Description | Size |
|------|-------------|------|
| `custom_silicon_chzsoft.html` | CHZ-Soft "Recovering Atari ST ASIC designs" article | ~12 KB |
| `custom_silicon_keli.html` | Keli.dk "Secret GAMECART Register" deep-dive | ~10 KB |
| `custom_silicon_hackaday.html` | Hackaday "Detective Work Recovers Atari ST ASIC Designs" article | ~130 KB |
| `custom_silicon_stee_hw.html` | Info-Coach "Atari STE Hardware" description (French ST Mag N44) | ~55 KB |
| `custom_silicon_atariage.html` | AtariAge "Atari ST ASIC designs and GAME SHIFTER" thread | ~5.5 KB |
| `custom_silicon_exxos.html` | Exxos "Atari Falcon custom chips decapsulated" thread | ~75 KB |
| `falcon_decap_atariage.html` | AtariAge "Falcon custom chips decapsulated" thread | ~5.7 KB |
| `falcon_decap_atari_forum.html` | Atari-Forum "Falcon custom ICs decap" thread | ~102 KB |
| `custom_silicon_zerkman.html` | Zerkman "Building an Atari ST on FPGA" blog | ~16 KB |
| `gstmcu_github.html` | GitHub repository github.com/gyurco/gstmcu (cycle-accurate Verilog) | ~245 KB |
| `miester_atarist.html` | GitHub MiSTer-devel/AtariST_MiSTer (cycle-accurate FPGA core) | ~279 KB |
| `miester_gstmcu.html` | Alternative GST MCU reference page | ~245 KB |
| `hatari_manual.html` | Hatari emulator user's manual | ~137 KB |
| `steem_sse_source.html` | Steem SSE emulator SourceForge page | ~112 KB |
| `megaste_inside_goto10.html` | Goto 10 Retro "Inside the Atari Mega STe" article | ~151 KB |
| `vhdl_system_on_chip.pdf` | "Atari ST System-on-Chip in VHDL" university project (Ch.5) | ~4.1 MB |
| `atari_st_internals.pdf` | "Atari ST Internals" - Ch. 1.2-1.5 (GLUE, MMU, DMA, SHIFTER) | ~11 MB |
| `anatomy_atari_st.pdf` | "Anatomy of the Atari ST" ultra-detailed schematics | ~18.8 MB |
| `vme_spec_tt030_megaste.pdf` | "Atari VME Expansion for TT030 and Mega STE" official spec | ~127 KB |
