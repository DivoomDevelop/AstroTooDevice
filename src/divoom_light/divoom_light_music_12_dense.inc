/*
 * 12 模式高密度渲染层。
 * 这里有意复用主文件的整数 RGB565 图元，在低能量输入下仍维持完整主体，
 * 再由真实频段改变长度、宽度、半径和相位，避免退化成稀疏调试线框。
 */

static void divoom_light_music_expand_legacy_frame(
    Pdivoom_light_music_ctrl handle, uint16 source_width);

static void divoom_light_music_dense_get_color(uint8 index, uint8 gain,
    divoom_light_color_info *color)
{
    divoom_light_music_scale_color(&divoom_light_music_forge_color[index % 8U],
        gain, color);
}

static void divoom_light_music_dense_line(int16 x0, int16 y0, int16 x1,
    int16 y1, uint8 half_width, const divoom_light_color_info *color,
    uint8 *disp_buf)
{
    int16 offset;

    for(offset = (int16)-(int16)half_width;
        offset <= (int16)half_width; offset++)
    {
        divoom_light_music_draw_forge_line((int16)(x0 + offset), y0,
            (int16)(x1 + offset), y1, color, disp_buf, 1U);
        if(offset != 0)
        {
            divoom_light_music_draw_forge_line(x0, (int16)(y0 + offset),
                x1, (int16)(y1 + offset), color, disp_buf, 1U);
        }
    }
}

static void divoom_light_music_dense_glow_block(int16 x, int16 y,
    uint8 radius, const divoom_light_color_info *color, uint8 *disp_buf)
{
    divoom_light_color_info dim_color;
    divoom_light_color_info mid_color;

    divoom_light_music_scale_color(color, 54U, &dim_color);
    divoom_light_music_scale_color(color, 146U, &mid_color);
    divoom_light_music_draw_forge_block(x, y, (uint8)(radius + 3U),
        &dim_color, disp_buf);
    divoom_light_music_draw_forge_block(x, y, (uint8)(radius + 1U),
        &mid_color, disp_buf);
    divoom_light_music_draw_forge_block(x, y, radius, color, disp_buf);
}

static void divoom_light_music_dense_glow_diamond(int16 x, int16 y,
    uint8 radius, const divoom_light_color_info *color, uint8 *disp_buf)
{
    divoom_light_color_info dim_color;
    divoom_light_color_info mid_color;

    divoom_light_music_scale_color(color, 52U, &dim_color);
    divoom_light_music_scale_color(color, 148U, &mid_color);
    divoom_light_music_draw_forge_diamond(x, y, (uint8)(radius + 5U),
        &dim_color, disp_buf);
    divoom_light_music_draw_forge_diamond(x, y, (uint8)(radius + 2U),
        &mid_color, disp_buf);
    divoom_light_music_draw_forge_diamond(x, y, radius, color, disp_buf);
}

static void divoom_light_music_dense_ring(Pdivoom_light_music_ctrl handle,
    int16 center_x, int16 center_y, uint8 radius, uint8 color_index,
    uint8 dash_phase)
{
    divoom_light_color_info color;
    divoom_light_color_info dim_color;

    divoom_light_music_dense_get_color(color_index, 244U, &color);
    divoom_light_music_scale_color(&color, 72U, &dim_color);
    divoom_light_music_draw_concept_ring_color(center_x, center_y,
        (uint8)(radius - 2U), &dim_color, handle->disp_buf, dash_phase);
    divoom_light_music_draw_concept_ring_color(center_x, center_y,
        radius, &color, handle->disp_buf, dash_phase);
    divoom_light_music_draw_concept_ring_color(center_x, center_y,
        (uint8)(radius + 2U), &dim_color, handle->disp_buf, dash_phase);
}

/* 0：以多层发光轨道和密集轨道块建立明确的太阳核心。 */
static void divoom_light_music_dense_orbit(Pdivoom_light_music_ctrl handle)
{
    static const divoom_light_color_info core_white = {255U, 255U, 230U};
    uint16 ring;
    uint16 marker;
    uint8 radius;
    uint8 direction;
    uint8 level;
    int16 x;
    int16 y;
    divoom_light_color_info color;

    divoom_light_music_draw_concept_orbit(handle);
    for(ring = 0; ring < 4U; ring++)
    {
        radius = (uint8)(28U + ring * 24U);
        divoom_light_music_dense_ring(handle, 128, 128, radius,
            (uint8)(4U + ring), (uint8)(ring + handle->row_tick / 3U));
        for(marker = 0; marker < 12U; marker++)
        {
            if((marker + ring) % 3U == 1U)
            {
                continue;
            }
            direction = (uint8)((marker + ring * 3U +
                handle->row_tick / 4U) % 16U);
            x = (int16)(128 +
                divoom_light_music_forge_direction[direction][0] * radius / 64);
            y = (int16)(128 +
                divoom_light_music_forge_direction[direction][1] * radius / 64);
            divoom_light_music_dense_get_color((uint8)(4U + marker / 3U),
                232U, &color);
            divoom_light_music_draw_forge_block(x, y, 2U, &color,
                handle->disp_buf);
        }
    }
    level = divoom_light_music_get_concept_level(handle, 2U);
    divoom_light_music_dense_get_color(5U, 255U, &color);
    divoom_light_music_dense_glow_diamond(128, 128,
        (uint8)(16U + level / 10U), &color, handle->disp_buf);
    divoom_light_music_draw_forge_diamond(128, 128, 6U,
        &core_white, handle->disp_buf);
}

/* 第六种沿用第一种轨道语言，并扩充到画布边缘和四个能量门。 */
static void divoom_light_music_dense_orbit_expanded(
    Pdivoom_light_music_ctrl handle)
{
    uint16 gate;
    uint8 direction;
    uint8 level;
    int16 x;
    int16 y;
    int16 inner_x;
    int16 inner_y;
    divoom_light_color_info color;
    divoom_light_color_info dim_color;

    divoom_light_music_dense_orbit(handle);
    divoom_light_music_dense_ring(handle, 128, 128, 116U, 0U,
        (uint8)(handle->row_tick / 4U));
    for(gate = 0; gate < 8U; gate++)
    {
        direction = (uint8)(gate * 2U);
        level = divoom_light_music_get_concept_level(handle, gate * 8U);
        x = (int16)(128 +
            divoom_light_music_forge_direction[direction][0] * 112 / 64);
        y = (int16)(128 +
            divoom_light_music_forge_direction[direction][1] * 112 / 64);
        inner_x = (int16)(128 +
            divoom_light_music_forge_direction[direction][0] * 82 / 64);
        inner_y = (int16)(128 +
            divoom_light_music_forge_direction[direction][1] * 82 / 64);
        divoom_light_music_dense_get_color((uint8)(gate +
            handle->row_tick / 10U), 255U, &color);
        divoom_light_music_scale_color(&color, 68U, &dim_color);
        divoom_light_music_dense_line(inner_x, inner_y, x, y, 3U,
            &dim_color, handle->disp_buf);
        divoom_light_music_dense_line(inner_x, inner_y, x, y, 1U,
            &color, handle->disp_buf);
        divoom_light_music_dense_glow_block(x, y,
            level > 20U ? 4U : 3U, &color, handle->disp_buf);
    }
}

/* 1：补齐山体内部色阶和厚地平线，形成参考图中的双向能量体积。 */
static void divoom_light_music_dense_mountain(Pdivoom_light_music_ctrl handle)
{
    uint16 x;
    uint8 level;
    uint8 height;
    uint8 fill;
    int16 wave;
    int16 top_y;
    int16 bottom_y;
    divoom_light_color_info color;
    divoom_light_color_info dim_color;

    divoom_light_music_draw_concept_mountain(handle);
    for(x = 0; x < 256U; x += 3U)
    {
        level = divoom_light_music_get_concept_level(handle, x / 4U);
        wave = divoom_light_music_get_concept_wave(
            (uint16)(x / 2U + handle->row_tick / 2U), 26U);
        if(wave < 0)
        {
            wave = (int16)-wave;
        }
        height = (uint8)(24U + (uint8)wave + level / 2U);
        top_y = (int16)(126 - height);
        bottom_y = (int16)(130 + height);
        divoom_light_music_dense_get_color((uint8)(x / 32U), 255U, &color);
        divoom_light_music_scale_color(&color, 72U, &dim_color);
        for(fill = 0; fill < 4U; fill++)
        {
            divoom_light_music_dense_line((int16)x,
                (int16)(126 - fill * height / 4U), (int16)(x + 1U),
                (int16)(top_y + fill * height / 5U), 0U, &color,
                handle->disp_buf);
            divoom_light_music_dense_line((int16)x,
                (int16)(130 + fill * height / 4U), (int16)(x + 1U),
                (int16)(bottom_y - fill * height / 5U), 0U, &dim_color,
                handle->disp_buf);
        }
    }
    divoom_light_music_dense_get_color(3U, 255U, &color);
    divoom_light_music_dense_line(0, 128, 255, 128, 2U, &color,
        handle->disp_buf);
}

/* 2：每列采用暗尾、彩色主体和亮滴头，并增加三层落点光圈。 */
static void divoom_light_music_dense_rain(Pdivoom_light_music_ctrl handle)
{
    uint16 column;
    uint16 drop;
    uint8 level;
    int16 x;
    int16 y;
    int16 landing_y;
    int16 width;
    divoom_light_color_info color;
    divoom_light_color_info dim_color;

    divoom_light_music_draw_concept_rain(handle);
    for(column = 0; column < 12U; column++)
    {
        level = divoom_light_music_get_concept_level(handle, column * 5U);
        x = (int16)(10 + column * 21U);
        landing_y = (int16)(192 + column % 3U * 6U);
        divoom_light_music_dense_get_color((uint8)column, 255U, &color);
        divoom_light_music_scale_color(&color, 70U, &dim_color);
        for(drop = 0; drop < 11U + level / 8U; drop++)
        {
            y = (int16)((handle->row_tick * 4U + column * 13U + drop * 16U) %
                (uint16)landing_y);
            divoom_light_music_dense_line(x, (int16)(y - 6), x, y,
                drop % 3U == 0U ? 2U : 1U, &dim_color, handle->disp_buf);
            divoom_light_music_dense_glow_block(x, y,
                drop % 4U == 0U ? 2U : 1U, &color, handle->disp_buf);
        }
        for(drop = 0; drop < 3U; drop++)
        {
            width = (int16)(8 + drop * 8U + level / 7U);
            divoom_light_music_dense_line((int16)(x - width),
                (int16)(landing_y + drop * 9U), (int16)(x + width),
                (int16)(landing_y + drop * 9U), 1U,
                drop == 0U ? &color : &dim_color, handle->disp_buf);
        }
    }
}

/* 3：六条旋臂使用发光像素簇填满主体，不再只是中心向外的散点。 */
static void divoom_light_music_dense_galaxy(Pdivoom_light_music_ctrl handle)
{
    static const divoom_light_color_info core_white = {236U, 255U, 255U};
    uint16 arm;
    uint16 radius;
    uint8 direction;
    uint8 level;
    int16 x;
    int16 y;
    int16 previous_x;
    int16 previous_y;
    uint8 has_previous;
    divoom_light_color_info color;
    divoom_light_color_info dim_color;

    divoom_light_music_draw_concept_galaxy(handle);
    for(arm = 0; arm < 6U; arm++)
    {
        has_previous = 0U;
        previous_x = 128;
        previous_y = 128;
        for(radius = 15U; radius < 120U; radius += 5U)
        {
            direction = (uint8)((radius / 8U + arm * 3U +
                handle->row_tick / 3U) % 16U);
            level = divoom_light_music_get_concept_level(handle,
                arm * 10U + radius / 10U);
            x = (int16)(128 +
                divoom_light_music_forge_direction[direction][0] * radius / 64);
            y = (int16)(128 +
                divoom_light_music_forge_direction[direction][1] * radius / 64);
            divoom_light_music_dense_get_color((uint8)(arm + radius / 30U),
                244U, &color);
            divoom_light_music_scale_color(&color, 76U, &dim_color);
            if(has_previous)
            {
                divoom_light_music_dense_line(previous_x, previous_y, x, y,
                    2U, &dim_color, handle->disp_buf);
                divoom_light_music_dense_line(previous_x, previous_y, x, y,
                    0U, &color, handle->disp_buf);
            }
            divoom_light_music_dense_glow_block(x, y,
                (uint8)(1U + level / 24U), &color, handle->disp_buf);
            previous_x = x;
            previous_y = y;
            has_previous = 1U;
        }
    }
    divoom_light_music_dense_get_color(0U, 255U, &color);
    divoom_light_music_dense_glow_diamond(128, 128, 12U, &color,
        handle->disp_buf);
    divoom_light_music_draw_forge_diamond(128, 128, 5U,
        &core_white, handle->disp_buf);
}

/* 第十二种采用第四种星系，并叠加反向外旋臂形成更大的双层漩涡。 */
static void divoom_light_music_dense_galaxy_expanded(
    Pdivoom_light_music_ctrl handle)
{
    uint16 arm;
    uint16 radius;
    uint8 direction;
    uint8 level;
    int16 x;
    int16 y;
    int16 previous_x;
    int16 previous_y;
    uint8 has_previous;
    divoom_light_color_info color;
    divoom_light_color_info dim_color;

    divoom_light_music_dense_galaxy(handle);
    for(arm = 0; arm < 4U; arm++)
    {
        has_previous = 0U;
        previous_x = 128;
        previous_y = 128;
        for(radius = 30U; radius < 128U; radius += 6U)
        {
            direction = (uint8)((31U - radius / 7U + arm * 4U +
                handle->row_tick / 2U) % 16U);
            level = divoom_light_music_get_concept_level(handle,
                arm * 16U + radius / 9U);
            x = (int16)(128 +
                divoom_light_music_forge_direction[direction][0] * radius / 64);
            y = (int16)(128 +
                divoom_light_music_forge_direction[direction][1] * radius / 64);
            divoom_light_music_dense_get_color((uint8)(7U - arm +
                radius / 26U), 230U, &color);
            divoom_light_music_scale_color(&color, 58U, &dim_color);
            if(has_previous)
            {
                divoom_light_music_dense_line(previous_x, previous_y, x, y,
                    2U, &dim_color, handle->disp_buf);
                divoom_light_music_dense_line(previous_x, previous_y, x, y,
                    0U, &color, handle->disp_buf);
            }
            if(radius % 12U == 0U)
            {
                divoom_light_music_dense_glow_block(x, y,
                    level > 24U ? 3U : 2U, &color, handle->disp_buf);
            }
            previous_x = x;
            previous_y = y;
            has_previous = 1U;
        }
    }
}

/* 4：用连续粗频谱肋填满八片花瓣，建立参考图中的大面积彩色主体。 */
static void divoom_light_music_dense_flower(Pdivoom_light_music_ctrl handle)
{
    uint16 petal;
    uint16 step;
    uint8 direction;
    uint8 normal_direction;
    uint8 level;
    uint8 half_width;
    int16 envelope;
    int16 x;
    int16 y;
    int16 x0;
    int16 y0;
    int16 x1;
    int16 y1;
    divoom_light_color_info color;
    divoom_light_color_info dim_color;

    divoom_light_music_draw_concept_flower(handle);
    for(petal = 0; petal < 8U; petal++)
    {
        direction = (uint8)(petal * 2U);
        normal_direction = (uint8)((direction + 4U) % 16U);
        level = divoom_light_music_get_concept_level(handle, petal * 8U);
        for(step = 34U; step < 102U + level / 3U; step += 4U)
        {
            envelope = (int16)((int16)(step - 30U) *
                (int16)(112 - (int16)step) / 90);
            if(envelope < 0)
            {
                envelope = 0;
            }
            half_width = (uint8)(6U + envelope + level / 10U);
            x = (int16)(128 +
                divoom_light_music_forge_direction[direction][0] * step / 64);
            y = (int16)(128 +
                divoom_light_music_forge_direction[direction][1] * step / 64);
            x0 = (int16)(x -
                divoom_light_music_forge_direction[normal_direction][0] *
                half_width / 64);
            y0 = (int16)(y -
                divoom_light_music_forge_direction[normal_direction][1] *
                half_width / 64);
            x1 = (int16)(x + (x - x0));
            y1 = (int16)(y + (y - y0));
            divoom_light_music_dense_get_color((uint8)(petal + step / 16U),
                255U, &color);
            divoom_light_music_scale_color(&color, 64U, &dim_color);
            divoom_light_music_dense_line(x0, y0, x1, y1, 2U,
                &dim_color, handle->disp_buf);
            divoom_light_music_dense_line(x0, y0, x1, y1, 1U,
                &color, handle->disp_buf);
        }
    }
    divoom_light_music_dense_get_color(0U, 255U, &color);
    divoom_light_music_dense_glow_diamond(128, 128, 8U, &color,
        handle->disp_buf);
}

/* 5：将七条细折线扩展为有暗边和亮芯的分层波带。 */
static void divoom_light_music_dense_river(Pdivoom_light_music_ctrl handle)
{
    uint16 layer;
    uint16 x;
    uint8 level;
    int16 y;
    int16 previous_y;
    divoom_light_color_info color;
    divoom_light_color_info dim_color;

    divoom_light_music_draw_concept_river(handle);
    for(layer = 0; layer < 7U; layer++)
    {
        divoom_light_music_dense_get_color((uint8)(layer +
            handle->row_tick / 12U), 238U, &color);
        divoom_light_music_scale_color(&color, 62U, &dim_color);
        previous_y = (int16)(26 + layer * 34U);
        for(x = 3U; x < 256U; x += 3U)
        {
            level = divoom_light_music_get_concept_level(handle,
                x / 4U + layer * 5U);
            y = (int16)(26 + layer * 34U +
                divoom_light_music_get_concept_wave((uint16)(x / 2U +
                layer * 9U + handle->row_tick / 2U),
                (uint8)(10U + level / 6U)));
            divoom_light_music_dense_line((int16)(x - 3U), previous_y,
                (int16)x, y, 3U, &dim_color, handle->disp_buf);
            divoom_light_music_dense_line((int16)(x - 3U), previous_y,
                (int16)x, y, 1U, &color, handle->disp_buf);
            previous_y = y;
        }
    }
}

/* 6：填充建筑立面、楼顶高光、窗灯和纵向倒影。 */
static void divoom_light_music_dense_city(Pdivoom_light_music_ctrl handle)
{
    uint16 building;
    uint16 fill_x;
    uint16 window_y;
    uint8 level;
    uint8 height;
    int16 x0;
    int16 x1;
    int16 top_y;
    divoom_light_color_info color;
    divoom_light_color_info fill_color;
    divoom_light_color_info reflection_color;

    divoom_light_music_draw_concept_city(handle);
    for(building = 0; building < 20U; building++)
    {
        level = divoom_light_music_get_concept_level(handle, building * 3U);
        height = (uint8)(32U + (building * 17U + handle->row_tick / 3U) % 48U +
            level / 2U);
        x0 = (int16)(3 + building * 13U);
        x1 = (int16)(x0 + 9);
        top_y = (int16)(151 - height);
        divoom_light_music_dense_get_color((uint8)(building / 3U), 255U,
            &color);
        divoom_light_music_scale_color(&color, 62U, &fill_color);
        divoom_light_music_scale_color(&color, 42U, &reflection_color);
        for(fill_x = (uint16)(x0 + 1); fill_x < (uint16)x1; fill_x += 2U)
        {
            divoom_light_music_dense_line((int16)fill_x, top_y,
                (int16)fill_x, 151, 1U, &fill_color, handle->disp_buf);
            divoom_light_music_dense_line((int16)fill_x, 157,
                (int16)fill_x, (int16)(157 + height / 2U), 0U,
                &reflection_color, handle->disp_buf);
        }
        divoom_light_music_dense_line(x0, top_y, x1, top_y, 1U, &color,
            handle->disp_buf);
        for(window_y = (uint16)(top_y + 6); window_y < 149U; window_y += 10U)
        {
            divoom_light_music_draw_forge_block((int16)(x0 + 3),
                (int16)window_y, 1U, &color, handle->disp_buf);
            divoom_light_music_draw_forge_block((int16)(x0 + 7),
                (int16)(window_y + 3U), 1U, &color, handle->disp_buf);
        }
    }
}

/* 7：加粗同心圈、扫描扇区和目标光晕，形成完整雷达盘。 */
static void divoom_light_music_dense_radar(Pdivoom_light_music_ctrl handle)
{
    static const divoom_light_color_info radar_green = {74U, 255U, 82U};
    static const divoom_light_color_info radar_dim = {12U, 88U, 30U};
    uint16 ring;
    uint16 target;
    uint8 direction;
    uint8 side_direction;
    uint8 radius;
    int16 x;
    int16 y;

    divoom_light_music_draw_concept_radar(handle);
    for(ring = 0; ring < 4U; ring++)
    {
        radius = (uint8)(28U + ring * 29U);
        divoom_light_music_draw_concept_ring_color(128, 128, radius,
            &radar_green, handle->disp_buf, (uint8)(ring + handle->row_tick / 5U));
        divoom_light_music_draw_concept_ring_color(128, 128,
            (uint8)(radius + 2U), &radar_dim, handle->disp_buf, (uint8)ring);
    }
    direction = (uint8)((handle->row_tick / 2U) % 16U);
    for(target = 0; target < 4U; target++)
    {
        side_direction = (uint8)((direction + target) % 16U);
        x = (int16)(128 +
            divoom_light_music_forge_direction[side_direction][0] * 116 / 64);
        y = (int16)(128 +
            divoom_light_music_forge_direction[side_direction][1] * 116 / 64);
        divoom_light_music_dense_line(128, 128, x, y,
            target == 0U ? 2U : 0U,
            target == 0U ? &radar_green : &radar_dim, handle->disp_buf);
    }
    for(target = 0; target < 7U; target++)
    {
        direction = (uint8)((target * 5U + 1U) % 16U);
        radius = (uint8)(28U + target * 13U);
        x = (int16)(128 +
            divoom_light_music_forge_direction[direction][0] * radius / 64);
        y = (int16)(128 +
            divoom_light_music_forge_direction[direction][1] * radius / 64);
        divoom_light_music_dense_glow_block(x, y, 2U, &radar_green,
            handle->disp_buf);
    }
}

/* 8：每根爆发光束增加暗色外沿、亮芯和多级流星节点。 */
static void divoom_light_music_dense_starburst(Pdivoom_light_music_ctrl handle)
{
    static const divoom_light_color_info core_white = {255U, 255U, 236U};
    uint16 ray;
    uint16 marker;
    uint8 level;
    uint8 length;
    int16 x0;
    int16 y0;
    int16 x1;
    int16 y1;
    divoom_light_color_info color;
    divoom_light_color_info dim_color;

    divoom_light_music_draw_concept_starburst(handle);
    for(ray = 0; ray < 16U; ray++)
    {
        level = divoom_light_music_get_concept_level(handle, ray * 4U);
        length = (uint8)(68U + (ray * 13U + handle->row_tick) % 42U +
            level / 2U);
        x0 = (int16)(128 +
            divoom_light_music_forge_direction[ray][0] * 12 / 64);
        y0 = (int16)(128 +
            divoom_light_music_forge_direction[ray][1] * 12 / 64);
        x1 = (int16)(128 +
            divoom_light_music_forge_direction[ray][0] * length / 64);
        y1 = (int16)(128 +
            divoom_light_music_forge_direction[ray][1] * length / 64);
        divoom_light_music_dense_get_color((uint8)(4U + ray / 5U), 255U,
            &color);
        divoom_light_music_scale_color(&color, 56U, &dim_color);
        divoom_light_music_dense_line(x0, y0, x1, y1, 3U, &dim_color,
            handle->disp_buf);
        divoom_light_music_dense_line(x0, y0, x1, y1, 1U, &color,
            handle->disp_buf);
        for(marker = 30U; marker < length; marker += 24U)
        {
            x1 = (int16)(128 +
                divoom_light_music_forge_direction[ray][0] * marker / 64);
            y1 = (int16)(128 +
                divoom_light_music_forge_direction[ray][1] * marker / 64);
            divoom_light_music_dense_glow_block(x1, y1, 2U, &color,
                handle->disp_buf);
        }
    }
    divoom_light_music_dense_glow_diamond(128, 128, 9U,
        &core_white, handle->disp_buf);
}

/* 9：双螺旋增加外晕、亮芯和密集横档。 */
static void divoom_light_music_dense_helix(Pdivoom_light_music_ctrl handle)
{
    uint16 y;
    uint8 level;
    int16 offset;
    int16 x_left;
    int16 x_right;
    divoom_light_color_info cyan;
    divoom_light_color_info magenta;
    divoom_light_color_info rung;

    divoom_light_music_draw_concept_helix(handle);
    divoom_light_music_dense_get_color(0U, 255U, &cyan);
    divoom_light_music_dense_get_color(3U, 255U, &magenta);
    divoom_light_music_dense_get_color(2U, 110U, &rung);
    for(y = 8U; y < 250U; y += 4U)
    {
        level = divoom_light_music_get_concept_level(handle, y / 4U);
        offset = divoom_light_music_get_concept_wave(
            (uint16)(y / 3U + handle->row_tick / 2U),
            (uint8)(38U + level / 5U));
        x_left = (int16)(128 + offset);
        x_right = (int16)(128 - offset);
        divoom_light_music_dense_glow_block(x_left, (int16)y, 2U,
            &cyan, handle->disp_buf);
        divoom_light_music_dense_glow_block(x_right, (int16)y, 2U,
            &magenta, handle->disp_buf);
        if(y % 12U == 8U)
        {
            divoom_light_music_dense_line(x_left, (int16)y, x_right,
                (int16)y, 1U, &rung, handle->disp_buf);
        }
    }
}

/* 10：将四个菱形节点扩展为带火焰、同心池沿和粗能量桥的熔岩池。 */
static void divoom_light_music_dense_lava(Pdivoom_light_music_ctrl handle)
{
    static const int16 pool_pos[4][2] =
    {
        {72, 70}, {181, 58}, {70, 183}, {184, 180}
    };
    static const divoom_light_color_info lava_red = {255U, 40U, 10U};
    static const divoom_light_color_info lava_orange = {255U, 132U, 8U};
    static const divoom_light_color_info lava_yellow = {255U, 240U, 48U};
    uint16 pool;
    uint16 flame;
    uint8 level;
    uint8 radius;
    int16 x;
    int16 top_y;

    divoom_light_music_draw_concept_lava(handle);
    divoom_light_music_dense_line(pool_pos[0][0], pool_pos[0][1],
        pool_pos[1][0], pool_pos[1][1], 4U, &lava_red, handle->disp_buf);
    divoom_light_music_dense_line(pool_pos[0][0], pool_pos[0][1],
        pool_pos[2][0], pool_pos[2][1], 3U, &lava_orange, handle->disp_buf);
    divoom_light_music_dense_line(pool_pos[1][0], pool_pos[1][1],
        pool_pos[3][0], pool_pos[3][1], 3U, &lava_orange, handle->disp_buf);
    divoom_light_music_dense_line(pool_pos[2][0], pool_pos[2][1],
        pool_pos[3][0], pool_pos[3][1], 4U, &lava_red, handle->disp_buf);
    for(pool = 0; pool < 4U; pool++)
    {
        level = divoom_light_music_get_concept_level(handle, pool * 15U);
        radius = (uint8)(22U + level / 5U +
            (handle->row_tick / 3U + pool * 2U) % 5U);
        divoom_light_music_draw_concept_ring_color(pool_pos[pool][0],
            pool_pos[pool][1], (uint8)(radius + 7U), &lava_red,
            handle->disp_buf, (uint8)pool);
        divoom_light_music_draw_concept_ring_color(pool_pos[pool][0],
            pool_pos[pool][1], (uint8)(radius + 3U), &lava_orange,
            handle->disp_buf, (uint8)(pool + 2U));
        divoom_light_music_dense_glow_diamond(pool_pos[pool][0],
            pool_pos[pool][1], (uint8)(radius / 2U), &lava_yellow,
            handle->disp_buf);
        for(flame = 0; flame < 7U; flame++)
        {
            x = (int16)(pool_pos[pool][0] - 18 + flame * 6U);
            top_y = (int16)(pool_pos[pool][1] - radius -
                10 - (flame * 9U + handle->row_tick) % 22U);
            divoom_light_music_dense_line(x,
                (int16)(pool_pos[pool][1] - radius / 2U), x, top_y,
                flame % 2U == 0U ? 2U : 1U,
                flame % 2U == 0U ? &lava_orange : &lava_yellow,
                handle->disp_buf);
        }
    }
}

/* 11：四轨示波器增加暗色外沿、彩色亮芯和主轴高光。 */
static void divoom_light_music_dense_scope(Pdivoom_light_music_ctrl handle)
{
    static const uint8 base_y[4] = {38U, 94U, 152U, 212U};
    uint16 trace;
    uint16 x;
    uint8 level;
    int16 y;
    int16 previous_y;
    divoom_light_color_info color;
    divoom_light_color_info dim_color;

    divoom_light_music_draw_concept_scope(handle);
    for(trace = 0; trace < 4U; trace++)
    {
        divoom_light_music_dense_get_color((uint8)(trace * 2U), 255U, &color);
        divoom_light_music_scale_color(&color, 52U, &dim_color);
        previous_y = base_y[trace];
        for(x = 2U; x < 256U; x += 2U)
        {
            level = divoom_light_music_get_concept_level(handle,
                x / 4U + trace * 11U);
            y = (int16)(base_y[trace] +
                divoom_light_music_get_concept_wave((uint16)(x *
                (trace + 1U) / 2U + handle->row_tick),
                (uint8)(12U + level / 6U)));
            divoom_light_music_dense_line((int16)(x - 2U), previous_y,
                (int16)x, y, 3U, &dim_color, handle->disp_buf);
            divoom_light_music_dense_line((int16)(x - 2U), previous_y,
                (int16)x, y, 1U, &color, handle->disp_buf);
            previous_y = y;
        }
    }
}

static void divoom_light_music_draw_mode_concept(Pdivoom_light_music_ctrl handle)
{
    switch(handle->disp_type)
    {
        case DIVOOM_DISP_MUSIC_CENTER_MODE:
        {
            divoom_light_music_dense_orbit(handle);
            break;
        }
        case DIVOOM_DISP_MUSIC_EQ2_MODE:
        {
            divoom_light_music_dense_mountain(handle);
            break;
        }
        case DIVOOM_DISP_MUSIC_EQ4_MODE:
        {
            divoom_light_music_dense_rain(handle);
            break;
        }
        case DIVOOM_DISP_MUSIC_NORMAL_MODE:
        {
            divoom_light_music_dense_galaxy(handle);
            break;
        }
        case DIVOOM_DISP_MUSIC_RANDOM_SQUARE2_MODE:
        {
            divoom_light_music_dense_flower(handle);
            break;
        }
        case DIVOOM_DISP_MUSIC_EQ6_MODE:
        {
            /* 第六项直接放大 CENTER_MODE 已经生成的 64 点原始帧。 */
            divoom_light_music_expand_legacy_frame(handle, SCREEN_WIDTH_64);
            break;
        }
        case DIVOOM_DISP_MUSIC_EQ7_MODE:
        {
            divoom_light_music_dense_city(handle);
            break;
        }
        case DIVOOM_DISP_MUSIC_EQ8_MODE:
        {
            divoom_light_music_dense_radar(handle);
            break;
        }
        case DIVOOM_DISP_MUSIC_RANDOM_SQUARE1_MODE:
        {
            divoom_light_music_dense_starburst(handle);
            break;
        }
        case DIVOOM_DISP_MUSIC_EQ10_MODE:
        {
            divoom_light_music_dense_helix(handle);
            break;
        }
        case DIVOOM_DISP_MUSIC_EQ11_MODE:
        {
            divoom_light_music_dense_lava(handle);
            break;
        }
        case DIVOOM_DISP_MUSIC_EQ12_MODE:
        {
            /* 第十二项直接放大 NORMAL_MODE 已经生成的 64 点原始帧。 */
            divoom_light_music_expand_legacy_frame(handle, SCREEN_WIDTH_64);
            break;
        }
        default:
        {
            divoom_light_music_dense_orbit(handle);
            break;
        }
    }
}
