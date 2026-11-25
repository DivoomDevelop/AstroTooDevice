/* Copyright (C) 2024 Free Software Foundation, Inc.
   Contributed by Alibaba Damo Academy Xuantie Team.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

/* Routines implemented in xuantie-pass-mcfg.cc.  */
rtl_opt_pass * make_pass_mcfg (gcc::context *ctxt);

/* Routines implemented in xuantie-ext-builtins.cc.  */
extern bool xt_rvm_check_builtin_call (location_t, vec<location_t>, unsigned int, tree, unsigned int, tree *);
extern tree xt_rvm_resolve_overloaded_builtin (location_t, unsigned int, tree, vec<tree, va_gc> *);

/* Routines implemented in xuantie-lib.cc.  */
extern int xt_get_vxrm_mode (rtx operand);

/* Routines implemented in xuantie-ext-builtins.cc.  */
extern void xt_rvm_handle_pragma_xtheadmatrix (void);

/* Mode bitfield for saturation mode control.  */
enum saturation_mode
{
  UTNMODE_OFF = 0,
  UTNMODE_ON = 1,
  UTNMODE_NONE = 2,
};

/* Rounding mode bitfield for fixed point VXRM.  */
enum rvm_fixed_point_rounding_mode
{
  XMXRM_RNU,
  XMXRM_RNE,
  XMXRM_RDN,
  XMXRM_ROD,
  XMXRM_NONE	/* TODO: Use a separate namespace. */
};

enum rvm_floating_point_rounding_mode
{
  XMFRM_RNE = 0,
  XMFRM_RTZ = 1,
  XMFRM_RDN = 2,
  XMFRM_RUP = 3,
  XMFRM_RMM = 4,
  XMFRM_NONE = 10,
};

enum rvm_saturation_mode
{
  XMSATEN_DISABLE = 0,
  XMSATEN_ENABLE = 1,
  XMSATEN_NONE = 2,
};

extern enum rvm_fixed_point_rounding_mode get_xmxrm_mode (rtx operand);
extern enum rvm_floating_point_rounding_mode get_xmfrm_mode (rtx operand);
extern enum rvm_saturation_mode get_xmsaten_mode (rtx operand);
