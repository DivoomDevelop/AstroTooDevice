# VS Simulator — Port layer source files

Add the following to the Visual Studio project / `product.props` ClCompile list:

```
lichee/rtos/projects/f101s3/astrotoo/src/port/sim_win/divoom_port_os_sim.c
lichee/rtos/projects/f101s3/astrotoo/src/port/sim_win/divoom_port_fs_sim.c
lichee/rtos/projects/f101s3/astrotoo/src/port/sim_win/divoom_port_hw_sim.c
lichee/rtos/projects/f101s3/astrotoo/src/port/common/divoom_port_fs_io.c
lichee/rtos/projects/f101s3/astrotoo/src/port/common/divoom_port_fs_tree.c
lichee/rtos/projects/f101s3/astrotoo/src/port/common/divoom_port_fs_exec.c
```

Add include path (if not already covered by middle includes):

```
lichee/rtos/projects/f101s3/astrotoo/src/port/include
```

**Exclude from simulator** (RTOS-only Finsh / I2S DIY 路径，与 `divoom_snd_pcm.c` 互斥):

```
lichee/rtos/projects/f101s3/astrotoo/src/port/rtos_f101/divoom_port_os_rtos.c
lichee/rtos/projects/f101s3/astrotoo/src/port/rtos_f101/divoom_port_fs_rtos.c
lichee/rtos/projects/f101s3/astrotoo/src/port/rtos_f101/divoom_port_hw_rtos.c
lichee/rtos/projects/f101s3/astrotoo/src/middle/divoom_snd_pcm_rtos.c
lichee/rtos/projects/f101s3/astrotoo/src/middle/divoom_snd_pcm_cli.c
lichee/rtos/projects/f101s3/astrotoo/src/middle/divoom_tone_cut_cli.c
```

**双构建共用**（`-DDIVOOM_SIMULATOR` 时走 `#elif DIVOOM_BUILD_SIMULATOR` 空实现分支，**无需**额外 stub 文件）：

```
lichee/rtos/projects/f101s3/astrotoo/src/middle/divoom_platform_audio_play.c
lichee/rtos/projects/f101s3/astrotoo/src/middle/divoom_audio_sink.c
lichee/rtos/projects/f101s3/astrotoo/src/middle/divoom_audio_policy.c
lichee/rtos/projects/f101s3/astrotoo/src/middle/divoom_sleep_pcm_session.c
```

**Simulator dependency:** `divoom_port_fs_sim.c` includes `sim_path_redirect.h` and calls
`sim_path_redirect(path)`. Ensure the simulator include path still provides that header.

**Removed from `divoom.h`:** `sim_path_redirect.h` and `sim_path_redirect_fd.h` are no longer
included globally. If other translation units relied on `sim_path_redirect_fd.h` macros for
`fopen`, include it locally in those files or route I/O through `divoom_port_fs.h`.
