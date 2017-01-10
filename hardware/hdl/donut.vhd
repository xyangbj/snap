----------------------------------------------------------------------------
----------------------------------------------------------------------------
--
-- Copyright 2016 International Business Machines
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions AND
-- limitations under the License.
--
----------------------------------------------------------------------------
----------------------------------------------------------------------------

LIBRARY ieee;--, ibm, ibm_asic;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;

--USE ibm_asic.std_ulogic_asic_function_support.all;
--USE ibm.std_ulogic_function_support.all;
--USE ibm.std_ulogic_support.all;

USE work.psl_accel_types.ALL;
USE work.donut_types.ALL;

ENTITY donut IS
  PORT (
    --
    -- PSL Interface
    --
    -- Command interface
    ah_cvalid       : OUT std_ulogic;                  -- Command valid
    ah_ctag         : OUT std_ulogic_vector(0 TO 7);   -- Command tag
    ah_ctagpar      : OUT std_ulogic;                  -- Command tag parity
    ah_com          : OUT std_ulogic_vector(0 TO 12);  -- Command code
    ah_compar       : OUT std_ulogic;                  -- Command code parity
    ah_cabt         : OUT std_ulogic_vector(0 TO 2);   -- Command ABT
    ah_cea          : OUT std_ulogic_vector(0 TO 63);  -- Command address
    ah_ceapar       : OUT std_ulogic;                  -- Command address parity
    ah_cch          : OUT std_ulogic_vector(0 TO 15);  -- Command context handle
    ah_csize        : OUT std_ulogic_vector(0 TO 11);  -- Command size
    ha_croom        : IN  std_ulogic_vector(0 TO 7);   -- Command room
    --
    -- Buffer interface
    ha_brvalid      : IN  std_ulogic;                  -- Buffer Read valid
    ha_brtag        : IN  std_ulogic_vector(0 TO 7);   -- Buffer Read tag
    ha_brtagpar     : IN  std_ulogic;                  -- Buffer Read tag parity
    ha_brad         : IN  std_ulogic_vector(0 TO 5);   -- Buffer Read address
    ah_brlat        : OUT std_ulogic_vector(0 TO 3);   -- Buffer Read latency
    ah_brdata       : OUT std_ulogic_vector(0 TO 511); -- Buffer Read data
    ah_brpar        : OUT std_ulogic_vector(0 TO 7);   -- Buffer Read parity
    ha_bwvalid      : IN  std_ulogic;                  -- Buffer Write valid
    ha_bwtag        : IN  std_ulogic_vector(0 TO 7);   -- Buffer Write tag
    ha_bwtagpar     : IN  std_ulogic;                  -- Buffer Write tag parity
    ha_bwad         : IN  std_ulogic_vector(0 TO 5);   -- Buffer Write address
    ha_bwdata       : IN  std_ulogic_vector(0 TO 511); -- Buffer Write data
    ha_bwpar        : IN  std_ulogic_vector(0 TO 7);   -- Buffer Write parity
    --
    --  Response interface
    ha_rvalid       : IN  std_ulogic;                  -- Response valid
    ha_rtag         : IN  std_ulogic_vector(0 TO 7);   -- Response tag
    ha_rtagpar      : IN  std_ulogic;                  -- Response tag parity
    ha_response     : IN  std_ulogic_vector(0 TO 7);   -- Response
    ha_rcredits     : IN  std_ulogic_vector(0 TO 8);   -- Response credits
    ha_rcachestate  : IN  std_ulogic_vector(0 TO 1);   -- Response cache state
    ha_rcachepos    : IN  std_ulogic_vector(0 TO 12);  -- Response cache pos
    --
    -- MMIO interface
    ha_mmval        : IN  std_ulogic;                  -- A valid MMIO is present
    ha_mmcfg        : IN  std_ulogic;                  -- MMIO is AFU descriptor space access
    ha_mmrnw        : IN  std_ulogic;                  -- 1 = read  0 = write
    ha_mmdw         : IN  std_ulogic;                  -- 1 = doubleword  0 = word
    ha_mmad         : IN  std_ulogic_vector(0 TO 23);  -- mmio address
    ha_mmadpar      : IN  std_ulogic;                  -- mmio address parity
    ha_mmdata       : IN  std_ulogic_vector(0 TO 63);  -- Write data
    ha_mmdatapar    : IN  std_ulogic;                  -- Write data parity
    ah_mmack        : OUT std_ulogic;                  -- Write is complete or Read is valid
    ah_mmdata       : OUT std_ulogic_vector(0 TO 63);  -- Read data
    ah_mmdatapar    : OUT std_ulogic;                  -- Read data parity
    --
    -- Control interface
    ha_jval         : IN  std_ulogic;                  -- Job valid
    ha_jcom         : IN  std_ulogic_vector(0 TO 7);   -- Job command
    ha_jcompar      : IN  std_ulogic;                  -- Job command parity
    ha_jea          : IN  std_ulogic_vector(0 TO 63);  -- Job address
    ha_jeapar       : IN  std_ulogic;                  -- Job address parity
    ah_jrunning     : OUT std_ulogic;                  -- Job running
    ah_jdone        : OUT std_ulogic;                  -- Job done
    ah_jcack        : OUT std_ulogic;                  -- Acknowledge completion of LLCMD
    ah_jerror       : OUT std_ulogic_vector(0 TO 63);  -- Job error
    ah_jyield       : OUT std_ulogic;                  -- Job yield
    ah_tbreq        : OUT std_ulogic := '0';           -- Timebase command request
    ah_paren        : OUT std_ulogic;                  -- Parity enable
    ha_pclock       : IN  std_ulogic;                  -- clock
    --
    -- ACTION Interface
    -- misc
    action_reset   : OUT std_ulogic;
    --
    -- Kernel AXI Master Interface
    xk_d_o         : OUT XK_D_T;
    kx_d_i         : IN  KX_D_T;
    --
    -- Kernel AXI Slave Interface
    sk_d_o         : OUT SK_D_T;
    ks_d_i         : IN  KS_D_T
    
  );
END donut;

ARCHITECTURE donut OF donut IS
  --
  -- CONSTANT

  --
  -- TYPE

  --
  -- ATTRIBUTE

  --
  -- SIGNAL
  SIGNAL afu_reset           : std_ulogic;
  SIGNAL ah_b                : AH_B_T;
  SIGNAL ah_c                : AH_C_T;
  SIGNAL ah_j                : AH_J_T;
  SIGNAL ah_mm               : AH_MM_T;
  SIGNAL cmm_e               : CMM_E_T;
  SIGNAL dmm_e               : DMM_E_T;
  SIGNAL ha_b                : HA_B_T;
  SIGNAL ha_c                : HA_C_T;
  SIGNAL ha_j                : HA_J_T;
  SIGNAL ha_mm               : HA_MM_T;
  SIGNAL ha_r                : HA_R_T;
  SIGNAL mmc_e               : MMC_E_T;
  SIGNAL mmd_a               : MMD_A_T;
  SIGNAL mmd_i               : MMD_I_T;
  SIGNAL mmx_d               : MMX_D_T;
  SIGNAL xmm_d               : XMM_D_T;

  signal xk_d                : XK_D_T;
  signal kx_d                : KX_D_T;
  signal sk_d                : SK_D_T;
  signal ks_d                : KS_D_T;

  signal sd_c                : SD_C_T;
  signal sd_d                : SD_D_T;
  signal ds_c                : DS_C_T;
  signal ds_d                : DS_D_T;


BEGIN

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- *******************************************************
-- ***** PSL<->AFU INTERFACE CONNECTION AND ENCODING *****
-- *******************************************************
--
-- Note: This section is necessary because ncsim and quartus interprets a
--       verilog to vhdl connection in different ways
--
--       tool    | verilog  |      vhdl    |  connection order
--       =====================================================================
--       ncsim:  | (0 to 7) | (7 downto 0) | left to left and  right to rigth
--       quartus | (0 to 7) | (7 downto 0) | 0 to 0       and 7 to 7
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  -- Command Interface
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
    ha_c.room(7 DOWNTO 0)       <= v2vhdl_connector(ha_croom(0 TO 7));

    ah_cvalid                   <=                  ah_c.valid;
    ah_ctag(0 TO 7)             <= vhdl2v_connector(ah_c.tag(7 DOWNTO 0));
    ah_ctagpar                  <=                  ah_c.tagpar;
    ah_com(0 TO 12)             <= ENCODE_CMD_CODES(ah_c.com);
    ah_compar                   <=                  ah_c.compar;
    ah_cabt(0 TO 2)             <= vhdl2v_connector(ah_c.abt(2 DOWNTO 0));
    ah_cea(0 TO 63)             <= vhdl2v_connector(ah_c.ea(63 DOWNTO 0));
    ah_ceapar                   <=                  ah_c.eapar;
    ah_cch(0 TO 15)             <= vhdl2v_connector(ah_c.ch(15 DOWNTO 0));
    ah_csize(0 TO 11)           <= vhdl2v_connector(ah_c.size(11 DOWNTO 0));


  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  -- Buffer Interface
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
    ha_b.rvalid                 <=                  ha_brvalid;
    ha_b.rtag(7 DOWNTO 0)       <= v2vhdl_connector(ha_brtag(0 TO 7));
    ha_b.rtagpar                <=                  ha_brtagpar;
    ha_b.rad(5 DOWNTO 0)        <= v2vhdl_connector(ha_brad(0 TO 5));
    ha_b.wvalid                 <=                  ha_bwvalid;
    ha_b.wtag(7 DOWNTO 0)       <= v2vhdl_connector(ha_bwtag(0 TO 7));
    ha_b.wtagpar                <=                  ha_bwtagpar;
    ha_b.wad(5 DOWNTO 0)        <= v2vhdl_connector(ha_bwad(0 TO 5));
    ha_b.wdata(511 DOWNTO 0)    <= v2vhdl_connector(ha_bwdata(0 TO 511));
    ha_b.wpar(7 DOWNTO 0)       <= v2vhdl_connector(ha_bwpar(0 TO 7));

    ah_brlat(0 TO 3)            <= vhdl2v_connector(ah_b.rlat(3 DOWNTO 0));
    ah_brdata(0 TO 511)         <= vhdl2v_connector(ah_b.rdata(511 DOWNTO 0));
    ah_brpar(0 TO 7)            <= vhdl2v_connector(ah_b.rpar(7 DOWNTO 0));


  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  -- Response Interface
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
    ha_r.valid                  <=                  ha_rvalid;
    ha_r.tag(7 DOWNTO 0)        <= v2vhdl_connector(ha_rtag(0 TO 7));
    ha_r.tagpar                 <=                  ha_rtagpar;
    ha_r.response               <= ENCODE_RSP_CODES(to_integer(unsigned(ha_response(0 TO 7))));
    ha_r.credits(8 DOWNTO 0)    <= v2vhdl_connector(ha_rcredits(0 TO 8));
    ha_r.cachestate(1 DOWNTO 0) <= v2vhdl_connector(ha_rcachestate(0 TO 1));
    ha_r.cachepos(12 DOWNTO 0)  <= v2vhdl_connector(ha_rcachepos(0 TO 12));


  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  -- MMIO Interface
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
    ha_mm.valid                 <=                  ha_mmval;
    ha_mm.cfg                   <=                  ha_mmcfg;
    ha_mm.rnw                   <=                  ha_mmrnw;
    ha_mm.dw                    <=                  ha_mmdw;
    ha_mm.ad(23 DOWNTO 0)       <= v2vhdl_connector(ha_mmad(0 TO 23));
    ha_mm.adpar                 <=                  ha_mmadpar;
    ha_mm.data(63 DOWNTO 0)     <= v2vhdl_connector(ha_mmdata(0 TO 63));
    ha_mm.datapar               <=                  ha_mmdatapar;

    ah_mmack                    <=                  ah_mm.ack;
    ah_mmdata(0 TO 63)          <= vhdl2v_connector(ah_mm.data(63 DOWNTO 0));
    ah_mmdatapar                <=                  ah_mm.datapar;


  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  -- Control Interface
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
    ha_j.valid                  <=                  ha_jval;
    ha_j.com                    <= ENCODE_COM_CODES(to_integer(unsigned(ha_jcom(0 TO 7))));
    ha_j.compar                 <=                  ha_jcompar;
    ha_j.ea(63 DOWNTO 0)        <= v2vhdl_connector(ha_jea(0 TO 63));
    ha_j.eapar                  <=                  ha_jeapar;

    ah_jrunning                 <=                  ah_j.running;
    ah_jdone                    <=                  ah_j.done;
    ah_jcack                    <=                  ah_j.cack;
    ah_jerror(0 TO 63)          <= vhdl2v_connector(ah_j.error(63 DOWNTO 0));
    ah_jyield                   <=                  ah_j.yield;



--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- ******************************************************
-- ***** DONUT ENTITIES                             *****
-- ******************************************************
--
--------------------------------------------------------------------------------
-------------------------------------------------------------------------------

  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  -- DMA Entity
  --
  --
  -- shortcut = d
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
    dma: ENTITY work.dma
    PORT MAP (
      --
      -- pervasive
      ha_pclock              => ha_pclock,
      afu_reset              => afu_reset,
      --
      -- PSL Interface
      ha_c_i                 => ha_c,
      ha_r_i                 => ha_r,
      ha_b_i                 => ha_b,
      ah_c_o                 => ah_c,
      ah_b_o                 => ah_b,
      --
      -- AXI SLAVE Interface
      sd_c_i                 => sd_c,
      ds_c_o                 => ds_c,
      sd_d_i                 => sd_d,
      ds_d_o                 => ds_d
    );


  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  -- CTRL MANAGER Entity
  --
  --
  -- shortcut = c
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
    ctrl_mgr: ENTITY work.ctrl_mgr
    PORT MAP (
      --
      -- pervasive
      ha_pclock              => ha_pclock,

      --
      -- PSL IOs
      ha_j_i                 => ha_j,
      ah_j_o                 => ah_j,

      --
      -- Global AFU Signals
      afu_reset_o            => afu_reset,
      --
      -- MMIO IOs
      mmc_e_i                => mmc_e,
      cmm_e_o                => cmm_e
    );


  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  -- MMIO Entity
  --
  --
  -- shortcut = m
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
    mmio: ENTITY work.mmio
    PORT MAP (
      --
      -- pervasive
      ha_pclock              => ha_pclock,
      afu_reset              => afu_reset,
      --
      -- debug (TODO: remove)
      ah_paren_mm_o          => ah_paren,
      --
      -- PSL IOs
      ha_mm_i                => ha_mm,
      ah_mm_o                => ah_mm,
      --
      -- CTRL MGR Interface
      cmm_e_i                => cmm_e,
      mmc_e_o                => mmc_e,
      --
      -- DMA Interface
      dmm_e_i                => dmm_e,
      mmd_a_o                => mmd_a,
      mmd_i_o                => mmd_i,
      --
      -- AXI MASTER Interface
      xmm_d_i                => xmm_d,
      mmx_d_o                => mmx_d
    );


  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
  -- AXI MASTER Entity
  --
  --
  -- shortcut = x
  ------------------------------------------------------------------------------
  ------------------------------------------------------------------------------
    mmio_to_axi_master: ENTITY work.mmio_to_axi_master
    PORT MAP (
      --
      -- pervasive
      clk                    => ha_pclock,
      rst                    => afu_reset,
      --
      -- MMIO Interface
      mmx_d_i                => mmx_d,
      xmm_d_o                => xmm_d,

      -- Application / Kernel Interface
      xk_d_o                 => xk_d_o,   -- axi master lite
      kx_d_i                 => kx_d_i
    );


   axi_dma_shim: ENTITY work.axi_dma_shim
    PORT MAP (
      --
      -- pervasive
      ha_pclock                    => ha_pclock,
      afu_reset                    => afu_reset,
      --
      sd_c_o                       => sd_c,
      sd_d_o                       => sd_d,
      ds_c_i                       => ds_c,
      ds_d_i                       => ds_d,

      sk_d_o                       => sk_d_o,  -- axi slave full interface
      ks_d_i                       => ks_d_i
     );

    action_reset <= afu_reset;
END ARCHITECTURE;
