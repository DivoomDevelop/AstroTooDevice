#include "divoom.h"
#include "divoom_radio_menu.h"
#include "divoom_lvgl_top_radio_wait.h"
#include "divoom_menu_control.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_main.h"
#include "divoom_lvgl9_compat.h"
#include <string.h>
#include <stdio.h>

/* 旧接口头缺少此已实现函数的声明；64位仿真必须保留完整指针。 */
extern void *divoom_menu_direct_ctrl_get_obj(void *handle, uint8 x, uint8 y);

#define DIVOOM_RADIO_MENU_ROWS 5
#define DIVOOM_RADIO_MENU_NAV_MAX 513
#define DIVOOM_RADIO_MENU_TAB_COUNT 3
#define DIVOOM_RADIO_MENU_TAB_FOCUS_BG 0xF4D9A2 /* 真源焦点色 40% 与 V1 底色混合 */
#define DIVOOM_RADIO_MENU_STATION_STEP 76
#define DIVOOM_RADIO_MENU_CATEGORY_STEP 80

enum
{
    DIVOOM_RADIO_MENU_LIST,
    DIVOOM_RADIO_MENU_TABS,
    DIVOOM_RADIO_MENU_FILTER,
    DIVOOM_RADIO_MENU_HEADER
};
/* 与点播等待浮层同尺寸/配色；root 相对整屏下移顶栏64+8，Y 对齐屏幕中心。 */

typedef struct
{
    const char *category;
    const char *empty;
    const char *change_category;
    const char *select_category;
    const char *disclaimer;
    const char *provider;
    const char *availability;
} divoom_radio_menu_text;

/* 页面专用文案沿用 AstroTooUI 各语言版本；接口类目名称原样显示。 */
static const divoom_radio_menu_text g_divoom_radio_menu_text[DIVOOM_LVGL_MENU_LANG_END] =
{
    [DIVOOM_LVGL_MENU_LANG_ENGLISH] = {"Genres", "No stations",
        "Categories", "Select category", "Disclaimer", "Internet radio programs are provided by third parties. Program content, broadcast times and availability depend on the service provided by each station.", "Some stations may be temporarily unavailable due to network conditions or regional restrictions."},
    [DIVOOM_LVGL_MENU_LANG_SC] = {"类目", "暂无电台",
        "切换类目", "选择类目", "免责声明", "网络电台节目由第三方提供。节目内容、播出时间及可用性以电台实际服务为准。", "部分电台可能因网络或地区限制暂时无法播放。"},
    [DIVOOM_LVGL_MENU_LANG_JP] = {"分類", "放送局なし",
        "カテゴリ変更", "カテゴリ選択", "免責事項", "インターネットラジオの番組は第三者が提供しています。番組内容、放送時間および利用可否は、各ラジオ局の実際のサービスに準じます。", "ネットワーク状況や地域制限により、一部のラジオ局を一時的に再生できない場合があります。"},
    [DIVOOM_LVGL_MENU_LANG_Thai] = {"หมวด", "ไม่มีสถานี",
        "เปลี่ยนหมวดหมู่", "เลือกหมวดหมู่", "ข้อจำกัดความรับผิด", "รายการวิทยุทางอินเทอร์เน็ตให้บริการโดยบุคคลที่สาม เนื้อหารายการ เวลาออกอากาศ และความพร้อมให้บริการขึ้นอยู่กับบริการจริงของแต่ละสถานี", "บางสถานีอาจไม่สามารถเล่นได้ชั่วคราวเนื่องจากสภาพเครือข่ายหรือข้อจำกัดในแต่ละภูมิภาค"},
    [DIVOOM_LVGL_MENU_LANG_French] = {"Genres", "Aucune station",
        "Catégories", "Choisir une catégorie", "Avertissement", "Les programmes de radio Internet sont fournis par des tiers. Le contenu, les horaires de diffusion et la disponibilité dépendent du service proposé par chaque station.", "Certaines stations peuvent être temporairement indisponibles en raison des conditions réseau ou de restrictions régionales."},
    [DIVOOM_LVGL_MENU_LANG_Italy] = {"Generi", "Nessuna stazione",
        "Categorie", "Scegli categoria", "Avvertenze", "I programmi delle radio Internet sono forniti da terzi. I contenuti, gli orari di trasmissione e la disponibilità dipendono dal servizio offerto da ciascuna emittente.", "Alcune emittenti potrebbero essere temporaneamente non disponibili a causa delle condizioni della rete o di restrizioni regionali."},
    [DIVOOM_LVGL_MENU_LANG_Vietnam] = {"Mục", "Không có đài",
        "Danh mục", "Chọn danh mục", "Tuyên bố miễn trừ trách nhiệm", "Các chương trình radio Internet do bên thứ ba cung cấp. Nội dung, thời gian phát sóng và tính khả dụng phụ thuộc vào dịch vụ thực tế của từng đài.", "Một số đài có thể tạm thời không phát được do tình trạng mạng hoặc hạn chế theo khu vực."},
    [DIVOOM_LVGL_MENU_LANG_Spanish] = {"Tipos", "No hay emisoras",
        "Categorías", "Elegir categoría", "Aviso legal", "Los programas de radio por Internet son proporcionados por terceros. El contenido, los horarios de emisión y la disponibilidad dependen del servicio de cada emisora.", "Algunas emisoras pueden no estar disponibles temporalmente debido a las condiciones de la red o a restricciones regionales."},
    [DIVOOM_LVGL_MENU_LANG_Germany] = {"Genres", "Keine Sender",
        "Kategorie", "Kategorie wählen", "Haftungsausschluss", "Internetradioprogramme werden von Drittanbietern bereitgestellt. Programminhalte, Sendezeiten und Verfügbarkeit richten sich nach dem tatsächlichen Angebot des jeweiligen Senders.", "Einige Sender sind aufgrund von Netzwerkbedingungen oder regionalen Einschränkungen möglicherweise vorübergehend nicht verfügbar."},
    [DIVOOM_LVGL_MENU_LANG_Russian] = {"Жанры", "Нет станций",
        "Категории", "Выберите категорию", "Отказ от ответственности", "Программы интернет-радио предоставляются третьими сторонами. Содержание, время вещания и доступность определяются фактическими услугами каждой радиостанции.", "Некоторые радиостанции могут быть временно недоступны из-за состояния сети или региональных ограничений."},
    [DIVOOM_LVGL_MENU_LANG_Portuguese] = {"Tipos", "Sem estações",
        "Categorias", "Escolher categoria", "Aviso legal", "Os programas de rádio pela Internet são fornecidos por terceiros. O conteúdo, os horários de transmissão e a disponibilidade dependem do serviço de cada estação.", "Algumas estações podem estar temporariamente indisponíveis devido às condições da rede ou a restrições regionais."},
    [DIVOOM_LVGL_MENU_LANG_Korean] = {"분류", "방송국 없음",
        "분류 변경", "분류 선택", "면책 안내", "인터넷 라디오 프로그램은 제3자가 제공합니다. 프로그램 내용, 방송 시간 및 이용 가능 여부는 각 방송국의 실제 서비스에 따라 달라집니다.", "네트워크 상태나 지역 제한으로 인해 일부 방송국을 일시적으로 재생하지 못할 수 있습니다."},
    [DIVOOM_LVGL_MENU_LANG_Swedish] = {"Genrer", "Inga stationer",
        "Kategorier", "Välj kategori", "Ansvarsfriskrivning", "Internetradioprogram tillhandahålls av tredje part. Programinnehåll, sändningstider och tillgänglighet beror på den tjänst som varje station erbjuder.", "Vissa stationer kan vara tillfälligt otillgängliga på grund av nätverksförhållanden eller regionala begränsningar."},
    [DIVOOM_LVGL_MENU_LANG_Ukraine] = {"Жанри", "Немає станцій",
        "Категорії", "Виберіть категорію", "Відмова від відповідальності", "Програми інтернет-радіо надаються третіми сторонами. Зміст, час мовлення та доступність залежать від фактичних послуг кожної радіостанції.", "Деякі радіостанції можуть бути тимчасово недоступні через стан мережі або регіональні обмеження."},
    [DIVOOM_LVGL_MENU_LANG_Danish] = {"Typer", "Ingen stationer",
        "Kategorier", "Vælg kategori", "Ansvarsfraskrivelse", "Internetradioprogrammer leveres af tredjeparter. Programindhold, sendetider og tilgængelighed afhænger af den tjeneste, som den enkelte station tilbyder.", "Nogle stationer kan være midlertidigt utilgængelige på grund af netværksforhold eller regionale begrænsninger."},
    [DIVOOM_LVGL_MENU_LANG_TC] = {"類別", "暫無電臺",
        "切換類別", "選擇類別", "免責聲明", "網路電臺節目由第三方提供。節目內容、播出時間及可用性以電臺實際服務為準。", "部分電臺可能因網路或地區限制暫時無法播放。"},
    [DIVOOM_LVGL_MENU_LANG_Turkey] = {"Türler", "İstasyon yok",
        "Kategoriler", "Kategori seç", "Sorumluluk reddi", "İnternet radyosu programları üçüncü taraflarca sağlanır. Program içeriği, yayın saatleri ve kullanılabilirlik, her istasyonun sunduğu hizmete bağlıdır.", "Bazı istasyonlar ağ koşulları veya bölgesel kısıtlamalar nedeniyle geçici olarak kullanılamayabilir."},
    [DIVOOM_LVGL_MENU_LANG_Indonesian] = {"Jenis", "Tidak ada stasiun",
        "Kategori", "Pilih kategori", "Penafian", "Program radio internet disediakan oleh pihak ketiga. Konten, jadwal siaran, dan ketersediaan bergantung pada layanan yang disediakan masing-masing stasiun.", "Beberapa stasiun mungkin tidak dapat diputar sementara karena kondisi jaringan atau pembatasan wilayah."},
    [DIVOOM_LVGL_MENU_LANG_Poland] = {"Typy", "Brak stacji",
        "Kategorie", "Wybierz kategorię", "Wyłączenie odpowiedzialności", "Programy radia internetowego są dostarczane przez podmioty zewnętrzne. Treść, godziny emisji i dostępność zależą od usług oferowanych przez daną stację.", "Niektóre stacje mogą być tymczasowo niedostępne z powodu warunków sieciowych lub ograniczeń regionalnych."},
};

static const divoom_radio_menu_text *divoom_radio_menu_get_text(void)
{
    int32 lang = divoom_main_set_lang_flag(-1);
    /* 初次开机尚未选语言时，与菜单加载器一样使用简体中文。 */
    if (lang == -1)
    {
        return &g_divoom_radio_menu_text[DIVOOM_LVGL_MENU_LANG_SC];
    }
    if (lang < 0 || lang >= DIVOOM_LVGL_MENU_LANG_END || !g_divoom_radio_menu_text[lang].category)
    {
        lang = DIVOOM_LVGL_MENU_LANG_ENGLISH;
    }
    return &g_divoom_radio_menu_text[lang];
}

typedef struct
{
    divoom_radio_menu_memory *memory;
    int32 menu_id;
    int32 rendered_key; /* 类目根页与子页共用控件，未绘制新行前禁止确认旧映射。 */
    lv_obj_t *root;
    lv_obj_t *title;
    lv_obj_t *info_button;
    lv_obj_t *info_heading;
    lv_obj_t *info_body;
    lv_obj_t *tabs[DIVOOM_RADIO_MENU_TAB_COUNT];
    lv_obj_t *underlines[DIVOOM_RADIO_MENU_TAB_COUNT];
    lv_obj_t *separator;
    lv_obj_t *filter_button;
    lv_obj_t *filter_path;
    lv_obj_t *viewport;
    lv_obj_t *extent;
    lv_obj_t *rows[DIVOOM_RADIO_MENU_ROWS];
    lv_obj_t *names[DIVOOM_RADIO_MENU_ROWS];
    lv_obj_t *details[DIVOOM_RADIO_MENU_ROWS];
    lv_obj_t *icons[DIVOOM_RADIO_MENU_ROWS];
    lv_obj_t *arrows[DIVOOM_RADIO_MENU_ROWS];
    lv_obj_t *checks[DIVOOM_RADIO_MENU_ROWS];
    lv_obj_t *favorites[DIVOOM_RADIO_MENU_ROWS];
    lv_obj_t *hearts[DIVOOM_RADIO_MENU_ROWS];
    lv_obj_t *empty;
    lv_obj_t *fail_panel;
    lv_obj_t *fail_frame;
    lv_obj_t *fail_lbl;
    lv_obj_t *dock;
    lv_obj_t *dock_label;
    lv_obj_t *dock_icon;
    void *direct;
    void *tab_direct;
    uint8 tab_nav_key;
    uint8 tab_nav_held; /* 连按预览之后的松手短按不能再次移动页签。 */
    uint16 tokens[DIVOOM_RADIO_MENU_NAV_MAX * 2];
    int32 map[DIVOOM_RADIO_MAX];
    int32 count;
    int32 window;
    int32 pending_focus;
    uint8 dirty;
    uint8 restore_scroll;
    uint8 append_move;
    uint8 china;
    uint8 category_open;
    uint8 info_open;
    uint8 select_current_category;
    uint32 hinted;
    uint32 favorite_error;
    divoom_radio_browse_state browse;
    divoom_radio_browse_view view;
    divoom_radio_station visible[DIVOOM_RADIO_MENU_ROWS];
} divoom_radio_menu_ctrl;

static void divoom_radio_menu_container(lv_obj_t *obj)
{
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

/* 与白噪音列表相同的填充/边框方式；选中框不再依赖额外PNG或其解码缓存。 */
static void divoom_radio_menu_card(lv_obj_t *obj, uint8 focused)
{
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(focused ? 0xf2b63a : 0xddd8d0), 0);
    lv_obj_set_style_border_width(obj, 2, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(focused ? 0xb8b0a8 : 0xd4cec6), 0);
    lv_obj_set_style_radius(obj, 12, 0);
}

static lv_obj_t *divoom_radio_menu_label(divoom_radio_menu_ctrl *ctrl, lv_obj_t *parent,
    int32 font, int32 x, int32 y, int32 width)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(font, ctrl->menu_id), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x2c2a27), 0);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_width(label, width);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    return label;
}

static void divoom_radio_menu_fail_set(divoom_radio_menu_ctrl *ctrl, uint8 show)
{
    lv_obj_t *shell;

    if (!ctrl || !ctrl->fail_panel)
    {
        return;
    }
    if (show)
    {
        if (ctrl->fail_lbl)
        {
            lv_label_set_text(ctrl->fail_lbl,
                divoom_menu_control_get_disp_str("NetworkConnectFail"));
        }
        divoom_astrootoo_ui_confirm_modal_layout(ctrl->fail_frame);
        shell = ctrl->fail_frame ? lv_obj_get_parent(ctrl->fail_frame) : NULL;
        if (shell)
        {
            lv_obj_center(shell);
        }
        lv_obj_clear_flag(ctrl->fail_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(ctrl->fail_panel);
    }
    else
    {
        lv_obj_add_flag(ctrl->fail_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

static uint8 divoom_radio_menu_list_loading(divoom_radio_menu_ctrl *ctrl, uint8 category)
{
    divoom_radio_menu_memory *memory = ctrl->memory;

    if (!category)
    {
        return ctrl->browse.applied != ctrl->browse.requested ||
            ctrl->view.load == DIVOOM_RADIO_LOADING;
    }
    if (ctrl->china)
    {
        return ctrl->browse.classify_load == DIVOOM_RADIO_LOADING;
    }
    if (memory->parent < 0)
    {
        return ctrl->browse.genre_load == DIVOOM_RADIO_LOADING;
    }
    return 0;
}

static uint8 divoom_radio_menu_list_failed(divoom_radio_menu_ctrl *ctrl, uint8 category)
{
    divoom_radio_menu_memory *memory = ctrl->memory;

    if (!category)
    {
        return ctrl->view.load == DIVOOM_RADIO_LOAD_FAILED;
    }
    if (ctrl->china)
    {
        return ctrl->browse.classify_load == DIVOOM_RADIO_LOAD_FAILED;
    }
    if (memory->parent < 0)
    {
        return ctrl->browse.genre_load == DIVOOM_RADIO_LOAD_FAILED;
    }
    return 0;
}

static void divoom_radio_menu_notice_create(divoom_radio_menu_ctrl *ctrl)
{
    lv_obj_t *shell;

    /* 与重置网络确认卡同一套壳：宽固定 408，正文换行后 SIZE_CONTENT 撑高并居中。 */
    ctrl->fail_panel = divoom_astrootoo_ui_confirm_modal_create(ctrl->root, &ctrl->fail_frame);
    if (!ctrl->fail_panel || !ctrl->fail_frame)
    {
        return;
    }
    lv_obj_set_size(ctrl->fail_panel, lv_obj_get_width(ctrl->root), lv_obj_get_height(ctrl->root));
    lv_obj_clear_flag(ctrl->fail_panel, LV_OBJ_FLAG_CLICKABLE);
    shell = lv_obj_get_parent(ctrl->fail_frame);
    if (shell)
    {
        lv_obj_clear_flag(shell, LV_OBJ_FLAG_CLICKABLE);
    }
    lv_obj_clear_flag(ctrl->fail_frame, LV_OBJ_FLAG_CLICKABLE);
    ctrl->fail_lbl = lv_label_create(ctrl->fail_frame);
    lv_label_set_long_mode(ctrl->fail_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ctrl->fail_lbl, LV_PCT(100));
    lv_obj_set_style_text_align(ctrl->fail_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(ctrl->fail_lbl, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(ctrl->fail_lbl,
        divoom_menu_control_get_font_info(24, ctrl->menu_id), 0);
    lv_label_set_text(ctrl->fail_lbl, divoom_menu_control_get_disp_str("NetworkConnectFail"));
    lv_obj_add_flag(ctrl->fail_panel, LV_OBJ_FLAG_HIDDEN);
}

static int32 divoom_radio_menu_view_key(divoom_radio_menu_ctrl *ctrl)
{
    /* 分类ID由服务决定，不能将其用作固定记忆数组下标。子类目共用一份行/滚动记忆。 */
    return ctrl->category_open ? (ctrl->memory->parent < 0 ? 3 : 4) : ctrl->memory->tab;
}



/* 既有国外分类的名称原样显示；不再把数据名称放进多语言文案。 */


static uint8 divoom_radio_menu_region_matches(divoom_radio_menu_ctrl *ctrl,
    const divoom_radio_browse_query *query)
{
    return query->kind == DIVOOM_RADIO_BROWSE_AUTO ||
        (query->kind == DIVOOM_RADIO_BROWSE_DOMESTIC) == ctrl->china;
}

static void divoom_radio_menu_request(divoom_radio_menu_ctrl *ctrl)
{
    divoom_radio_browse_query query = ctrl->memory->filter;
    if (ctrl->memory->tab == 1)
    {
        memset(&query, 0, sizeof(query));
        query.kind = DIVOOM_RADIO_BROWSE_FAVORITE;
    }
    else if (ctrl->memory->tab == 2)
    {
        memset(&query, 0, sizeof(query));
        query.kind = DIVOOM_RADIO_BROWSE_RECENT;
    }
    if (ctrl->category_open)
    {
        if (ctrl->china)
        {
            divoom_radio_browse_classify_request();
        }
        else
        {
            divoom_radio_browse_genre_request();
        }
        return;
    }
    if (ctrl->memory->tab == 0 && !divoom_radio_menu_region_matches(ctrl, &query))
    {
        memset(&query, 0, sizeof(query));
        query.kind = DIVOOM_RADIO_BROWSE_AUTO;
        ctrl->memory->filter = query;
        ctrl->memory->focus[0] = 0;
        ctrl->memory->scroll[0] = 0;
    }
    divoom_radio_browse_request(&query);
    if (ctrl->memory->tab == 0 && query.kind == DIVOOM_RADIO_BROWSE_DOMESTIC)
    {
        divoom_radio_browse_classify_request();
    }
}

static int32 divoom_radio_menu_category_count(divoom_radio_menu_ctrl *ctrl)
{
    if (ctrl->china)
    {
        return ctrl->browse.classify_count;
    }
    if (ctrl->memory->parent < 0)
    {
        return 1 + divoom_radio_browse_genre_count(0);
    }
    return 1 + divoom_radio_browse_genre_count(ctrl->memory->parent);
}

static void divoom_radio_menu_category_name(divoom_radio_menu_ctrl *ctrl, int32 row, char *text, size_t size)
{
    int32 parent = ctrl->memory->parent;
    int32 id;
    divoom_radio_browse_genre genre;
    text[0] = 0;
    if (ctrl->china)
    {
        divoom_radio_browse_classify_copy(row, &id, text, (uint32)size);
    }
    else if (parent < 0 && row == 0)
    {
        /* 固定榜单入口始终保留；它不是genre接口的分类，不添加分类翻译表。 */
        snprintf(text, size, "TOP100");
    }
    else if (parent > 0 && row == 0)
    {
        snprintf(text, size, "%s", divoom_menu_control_get_disp_str("SourceScopeAll"));
    }
    else if (divoom_radio_browse_genre_copy(parent < 0 ? 0 : parent,
        row - 1, &genre))
    {
        /* 接口名称直接显示，包括非英语；只处理XML实体，不做翻译替换。 */
        snprintf(text, size, "%s", genre.name);
    }
}

static void divoom_radio_menu_save_scroll(divoom_radio_menu_ctrl *ctrl)
{
    ctrl->memory->scroll[divoom_radio_menu_view_key(ctrl)] = lv_obj_get_scroll_y(ctrl->viewport);
}

static void divoom_radio_menu_apply_filter(divoom_radio_menu_ctrl *ctrl, const divoom_radio_browse_query *query)
{
    divoom_radio_menu_memory *memory = ctrl->memory;
    divoom_radio_menu_save_scroll(ctrl);
    ctrl->category_open = 0;
    ctrl->rendered_key = -1;
    memory->parent = -1;
    memory->filter = *query;
    memory->tab = 0;
    memory->pane = DIVOOM_RADIO_MENU_LIST;
    memory->focus[0] = 0;
    memory->scroll[0] = 0;
    memory->column[0] = 0;
    ctrl->append_move = 0;
    ctrl->restore_scroll = 1;
    divoom_radio_menu_request(ctrl);
}

/* 根类目的选中标记也能反映深一级的电台目录。 */
static uint8 divoom_radio_menu_category_selected(divoom_radio_menu_ctrl *ctrl, int32 row)
{
    const divoom_radio_browse_query *filter = &ctrl->memory->filter;
    int32 parent = ctrl->memory->parent;
    if (ctrl->china)
    {
        int32 id;
        char name[96];
        return filter->kind == DIVOOM_RADIO_BROWSE_DOMESTIC &&
            divoom_radio_browse_classify_copy(row, &id, name, sizeof(name)) && id == filter->classify_id;
    }
    if (parent < 0 && row == 0)
    {
        return filter->kind == DIVOOM_RADIO_BROWSE_TOP;
    }
    if (filter->kind != DIVOOM_RADIO_BROWSE_GENRE)
    {
        return 0;
    }
    if (parent > 0 && row == 0)
    {
        return !strcmp(filter->genre, ctrl->memory->parent_name);
    }
    divoom_radio_browse_genre genre;
    if (divoom_radio_browse_genre_copy(parent < 0 ? 0 : parent,
        row - 1, &genre))
    {
        if (!strcmp(filter->genre, genre.name))
        {
            return 1;
        }
        if (parent < 0)
        {
            int32 children = divoom_radio_browse_genre_count(genre.id);
            for (int32 i = 0; i < children; i++)
            {
                divoom_radio_browse_genre child;
                if (divoom_radio_browse_genre_copy(genre.id, i, &child) && !strcmp(filter->genre, child.name))
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}

static void divoom_radio_menu_filter_text(divoom_radio_menu_ctrl *ctrl, char *text, size_t size)
{
    const divoom_radio_browse_query *filter = &ctrl->memory->filter;
    if (filter->kind == DIVOOM_RADIO_BROWSE_DOMESTIC)
    {
        for (int32 i = 0; i < ctrl->browse.classify_count; i++)
        {
            int32 id;
            char name[96];
            if (divoom_radio_browse_classify_copy(i, &id, name, sizeof(name)) && id == filter->classify_id)
            {
                snprintf(text, size, "%s", name);
                return;
            }
        }
        snprintf(text, size, "%s", divoom_radio_menu_get_text()->category);
    }
    else
    {
        snprintf(text, size, "%s", filter->kind == DIVOOM_RADIO_BROWSE_GENRE ? filter->genre :
            filter->kind == DIVOOM_RADIO_BROWSE_TOP ? "TOP100" :
            divoom_menu_control_get_disp_str("SourceScopeAll"));
    }
}

static void divoom_radio_menu_show(lv_obj_t *obj, uint8 visible)
{
    if (visible)
    {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static void divoom_radio_menu_chrome(divoom_radio_menu_ctrl *ctrl)
{
    divoom_radio_menu_memory *memory = ctrl->memory;
    uint8 browse = !ctrl->category_open && !ctrl->info_open;
    uint8 header_focus = memory->pane == DIVOOM_RADIO_MENU_HEADER;
    uint8 filter_focus = memory->pane == DIVOOM_RADIO_MENU_FILTER;
    const char *title = ctrl->category_open ?
        (memory->parent < 0 ? divoom_radio_menu_get_text()->select_category : memory->parent_name) :
        divoom_menu_control_get_disp_str("SourceNetRadio");
    if (ctrl->title && strcmp(lv_label_get_text(ctrl->title), title))
    {
        lv_label_set_text(ctrl->title, title);
    }
    divoom_radio_menu_show(ctrl->info_button, !ctrl->category_open);
    lv_obj_set_style_bg_opa(ctrl->info_button, header_focus ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    for (int32 i = 0; i < DIVOOM_RADIO_MENU_TAB_COUNT; i++)
    {
        uint8 focused = browse && memory->pane == DIVOOM_RADIO_MENU_TABS && memory->tab == i;
        divoom_radio_menu_show(ctrl->tabs[i], browse);
        divoom_radio_menu_show(ctrl->underlines[i], browse && memory->tab == i);
        lv_obj_t *label = lv_obj_get_child(ctrl->tabs[i], 0);
        /* 页签底色由绘制回调处理，焦点变动时重画整个按钮。 */
        if (lv_obj_has_state(ctrl->tabs[i], LV_STATE_USER_1) != focused)
        {
            if (focused)
            {
                lv_obj_add_state(ctrl->tabs[i], LV_STATE_USER_1);
                lv_obj_add_state(label, LV_STATE_USER_1);
            }
            else
            {
                lv_obj_clear_state(ctrl->tabs[i], LV_STATE_USER_1);
                lv_obj_clear_state(label, LV_STATE_USER_1);
            }
            lv_obj_invalidate(ctrl->tabs[i]);
        }
        lv_obj_set_style_radius(ctrl->underlines[i], focused ? 4 : 2, 0);
        lv_obj_set_height(ctrl->underlines[i], focused ? 6 : 4);
        lv_obj_set_y(ctrl->underlines[i], focused ? 40 : 42);
        lv_obj_set_style_bg_color(ctrl->underlines[i],
            lv_color_hex(focused ? DIVOOM_ASTRO_SURFACE_FOCUS : DIVOOM_ASTRO_HEADER_V1), 0);
    }
    divoom_radio_menu_show(ctrl->separator, !ctrl->category_open);
    divoom_radio_menu_show(ctrl->filter_button, browse && memory->tab == 0);
    if (browse && memory->tab == 0)
    {
        char text[256];
        divoom_radio_menu_filter_text(ctrl, text, sizeof(text));
        if (strcmp(lv_label_get_text(ctrl->filter_path), text))
        {
            lv_label_set_text(ctrl->filter_path, text);
        }
        lv_label_set_long_mode(ctrl->filter_path, filter_focus ? LV_LABEL_LONG_SCROLL_CIRCULAR : LV_LABEL_LONG_DOT);
        lv_obj_set_style_bg_color(ctrl->filter_button,
            lv_color_hex(filter_focus ? DIVOOM_ASTRO_SURFACE_FOCUS : DIVOOM_ASTRO_SURFACE_IDLE), 0);
    }
    divoom_radio_menu_show(ctrl->viewport, !ctrl->info_open);
    divoom_radio_menu_show(ctrl->dock, browse);
    divoom_radio_menu_show(ctrl->info_heading, ctrl->info_open);
    divoom_radio_menu_show(ctrl->info_body, ctrl->info_open);
    lv_obj_add_flag(ctrl->empty, LV_OBJ_FLAG_HIDDEN);
}

/* 按 info-circle 真源尺寸画圆环、圆点和圆头竖线。焦点颜色与按钮底色同步。 */
static void divoom_radio_menu_draw_info(lv_event_t *event)
{
    lv_obj_t *button = lv_event_get_target(event);
    lv_layer_t *layer = lv_event_get_layer(event);
    lv_draw_rect_dsc_t shape;
    lv_area_t coords;
    lv_area_t ink_area;
    lv_color_t ink;
    if (!layer)
    {
        return;
    }

    lv_obj_get_coords(button, &coords);
    ink = lv_color_hex(lv_obj_get_style_bg_opa(button, LV_PART_MAIN) == LV_OPA_COVER ?
        DIVOOM_ASTRO_INK : DIVOOM_ASTRO_HEADER_INK);
    lv_draw_rect_dsc_init(&shape);
    shape.radius = LV_RADIUS_CIRCLE;
    shape.bg_opa = LV_OPA_TRANSP;
    shape.border_opa = LV_OPA_COVER;
    shape.border_color = ink;
    shape.border_width = 2;
    ink_area = (lv_area_t){coords.x1 + 12, coords.y1 + 12, coords.x1 + 35, coords.y1 + 35};
    lv_draw_rect(layer, &shape, &ink_area);

    shape.border_width = 0;
    shape.bg_color = ink;
    shape.bg_opa = LV_OPA_COVER;
    ink_area = (lv_area_t){coords.x1 + 23, coords.y1 + 18, coords.x1 + 24, coords.y1 + 19};
    lv_draw_rect(layer, &shape, &ink_area);
    ink_area = (lv_area_t){coords.x1 + 23, coords.y1 + 22, coords.x1 + 24, coords.y1 + 30};
    lv_draw_rect(layer, &shape, &ink_area);
}

/* 两次同色填充保留页签上 12 px、下 4 px 的不同圆角。 */
static void divoom_radio_menu_draw_tab(lv_event_t *event)
{
    lv_obj_t *tab = lv_event_get_target(event);
    lv_layer_t *layer = lv_event_get_layer(event);
    lv_draw_rect_dsc_t fill;
    lv_area_t area;
    if (!layer || !lv_obj_has_state(tab, LV_STATE_USER_1))
    {
        return;
    }

    lv_obj_get_coords(tab, &area);
    lv_draw_rect_dsc_init(&fill);
    fill.bg_color = lv_color_hex(DIVOOM_RADIO_MENU_TAB_FOCUS_BG);
    fill.bg_opa = LV_OPA_COVER;
    fill.radius = DIVOOM_ASTRO_RADIUS_SM;
    lv_draw_rect(layer, &fill, &area);
    area.y1 = area.y2 - 15;
    fill.radius = 4;
    lv_draw_rect(layer, &fill, &area);
}

static void divoom_radio_menu_create_chrome(divoom_radio_menu_ctrl *ctrl, lv_obj_t *header)
{
    const divoom_radio_menu_text *text = divoom_radio_menu_get_text();
    ctrl->title = lv_obj_get_child(header, -1);
    if (ctrl->title)
    {
        lv_obj_set_width(ctrl->title, 384);
        lv_obj_set_style_text_align(ctrl->title, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_long_mode(ctrl->title, LV_LABEL_LONG_DOT);
    }
    ctrl->info_button = lv_btn_create(header);
    divoom_radio_menu_container(ctrl->info_button);
    lv_obj_set_pos(ctrl->info_button, 412, 8);
    lv_obj_set_size(ctrl->info_button, 48, 48);
    lv_obj_set_style_radius(ctrl->info_button, DIVOOM_ASTRO_RADIUS_SM, 0);
    lv_obj_set_style_bg_color(ctrl->info_button, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
    lv_obj_add_event_cb(ctrl->info_button, divoom_radio_menu_draw_info, LV_EVENT_DRAW_MAIN, NULL);

    ctrl->separator = lv_obj_create(ctrl->root);
    divoom_radio_menu_container(ctrl->separator);
    lv_obj_set_pos(ctrl->separator, 4, 46);
    lv_obj_set_size(ctrl->separator, 472, 2);
    lv_obj_set_style_bg_color(ctrl->separator, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
    lv_obj_set_style_bg_opa(ctrl->separator, LV_OPA_COVER, 0);

    ctrl->filter_button = lv_btn_create(ctrl->root);
    divoom_radio_menu_container(ctrl->filter_button);
    lv_obj_set_pos(ctrl->filter_button, 4, 56);
    lv_obj_set_size(ctrl->filter_button, 472, 44);
    divoom_radio_menu_card(ctrl->filter_button, 0);
    lv_obj_t *arrow = lv_img_create(ctrl->filter_button);
    divoom_menu_control_lv_img_set_src(arrow, "23/at_sublevel_chevron.png");
    lv_img_set_angle(arrow, 900);
    lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -12, 0);
    lv_obj_t *action = divoom_radio_menu_label(ctrl, ctrl->filter_button, 16, 0, 0, LV_SIZE_CONTENT);
    lv_label_set_text(action, text->change_category);
    lv_obj_set_style_max_width(action, 200, 0);
    lv_obj_align(action, LV_ALIGN_RIGHT_MID, -32, 0);
    lv_obj_update_layout(action);
    ctrl->filter_path = divoom_radio_menu_label(ctrl, ctrl->filter_button, 16, 0, 0,
        410 - lv_obj_get_width(action));
    lv_obj_align(ctrl->filter_path, LV_ALIGN_LEFT_MID, 12, 0);

    ctrl->info_heading = divoom_radio_menu_label(ctrl, ctrl->root, 16, 4, 11, 472);
    lv_obj_set_style_text_align(ctrl->info_heading, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(ctrl->info_heading, text->disclaimer);
    ctrl->info_body = lv_obj_create(ctrl->root);
    divoom_radio_menu_container(ctrl->info_body);
    lv_obj_set_pos(ctrl->info_body, 4, 56);
    lv_obj_set_size(ctrl->info_body, 472, 344);
    lv_obj_set_style_pad_top(ctrl->info_body, 16, 0);
    lv_obj_set_style_pad_left(ctrl->info_body, 20, 0);
    lv_obj_set_style_pad_right(ctrl->info_body, 20, 0);
    lv_obj_set_style_pad_row(ctrl->info_body, 24, 0);
    lv_obj_set_flex_flow(ctrl->info_body, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag(ctrl->info_body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ctrl->info_body, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(ctrl->info_body, LV_SCROLLBAR_MODE_OFF);
    const char *paragraphs[] = {text->provider, text->availability};
    for (int32 i = 0; i < 2; i++)
    {
        lv_obj_t *label = divoom_radio_menu_label(ctrl, ctrl->info_body, 20, 0, 0, 432);
        const lv_font_t *font = lv_obj_get_style_text_font(label, 0);
        lv_obj_set_style_text_line_space(label, 32 - lv_font_get_line_height(font), 0);
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_label_set_text(label, paragraphs[i]);
    }
}

void divoom_radio_menu_refresh(void *handle)
{
    divoom_radio_menu_ctrl *ctrl = handle;
    divoom_radio_menu_memory *memory;
    int32 key, focus, count, start, visible, stride, indices[DIVOOM_RADIO_MENU_ROWS];
    uint8 category, changed;
    char text[256];
    if (!ctrl)
    {
        return;
    }
    memory = ctrl->memory;
    uint8 china = divoom_main_get_server_type() == 0;
    if (ctrl->china != china)
    {
        ctrl->china = china;
        memory->parent = -1;
        memory->focus[3] = 0;
        memory->scroll[3] = 0;
        ctrl->select_current_category = ctrl->category_open;
        ctrl->restore_scroll = 1;
        if (ctrl->category_open || memory->tab == 0)
        {
            divoom_radio_menu_request(ctrl);
        }
        ctrl->dirty = 1;
    }
    divoom_radio_browse_state browse;
    divoom_radio_browse_poll(&browse);
    changed = memcmp(&browse, &ctrl->browse, sizeof(browse)) != 0;
    ctrl->browse = browse;
    if (memory->filter.kind == DIVOOM_RADIO_BROWSE_AUTO && browse.requested == browse.applied &&
        browse.active_query.kind <= DIVOOM_RADIO_BROWSE_DOMESTIC &&
        divoom_radio_menu_region_matches(ctrl, &browse.active_query))
    {
        /* 只接收worker已提交的初始列表，不抢占用户随后点选的分类。 */
        memory->filter = browse.active_query;
        if (ctrl->china && !browse.classify_count)
        {
            divoom_radio_browse_classify_request();
        }
    }
    changed |= divoom_radio_browse_view_copy(&ctrl->view, ctrl->view.revision, ctrl->dirty);
    if (!ctrl->dirty && !changed)
    {
        return;
    }
    /* 行快照拷贝可能因后台版本推进失败；半次刷新期间也不能沿用上次可操作的映射。 */
    ctrl->rendered_key = -1;
    ctrl->dirty = 0;
    category = ctrl->category_open;
    divoom_radio_menu_chrome(ctrl);
    if (ctrl->info_open)
    {
        divoom_radio_menu_fail_set(ctrl, 0);
        /* 说明页独立使用自己的滚动区，浏览快照和目录焦点留在后台。 */
        divoom_lvgl_top_radio_wait_set_catalog(0, 0, 0);
        return;
    }
    stride = category ? DIVOOM_RADIO_MENU_CATEGORY_STEP : DIVOOM_RADIO_MENU_STATION_STEP;
    key = divoom_radio_menu_view_key(ctrl);
    count = 0;
    if (category)
    {
        count = divoom_radio_menu_category_count(ctrl);
        if (count > DIVOOM_RADIO_MENU_NAV_MAX)
        {
            count = DIVOOM_RADIO_MENU_NAV_MAX;
        }
    }
    else if (browse.applied == browse.requested && (memory->tab != 0 ||
        divoom_radio_menu_region_matches(ctrl, &browse.active_query)))
    {
        for (int32 i = 0; i < ctrl->view.count && count < DIVOOM_RADIO_MAX; i++)
        {
            if (ctrl->view.items[i].id != browse.pinned_id &&
                (memory->tab != 1 || ctrl->view.items[i].favorite))
            {
                ctrl->map[count++] = i;
            }
        }
        /* 最近页沿用后端本次进入时的顺序；成功点播更新历史不能挪动正在浏览的行。 */
    }
    ctrl->count = count;
    /* 保留真机的目录加载判定，等待动画交给起播共用浮层。 */
    uint8 loading = divoom_radio_menu_list_loading(ctrl, category);
    divoom_lvgl_top_radio_wait_set_catalog(loading, key, browse.requested);
    if (category && ctrl->select_current_category)
    {
        for (int32 i = 0; i < count; i++)
        {
            if (divoom_radio_menu_category_selected(ctrl, i))
            {
                memory->focus[key] = i;
                ctrl->restore_scroll = 1;
                break;
            }
        }
        if (ctrl->china ? browse.classify_load == DIVOOM_RADIO_READY :
            memory->parent > 0 || browse.genre_load == DIVOOM_RADIO_READY)
        {
            ctrl->select_current_category = 0;
        }
    }
    focus = memory->focus[key];
    if (ctrl->restore_scroll)
    {
        ctrl->pending_focus = focus;
    }
    if (ctrl->pending_focus >= 0)
    {
        focus = ctrl->pending_focus;
    }
    /* 浏览恢复超过已加载段时仍按6条补段；不因首段回包把旧焦点永久截断。 */
    if (!category && focus >= count && ctrl->view.count < ctrl->view.total &&
        !ctrl->view.more_error && browse.applied == browse.requested)
    {
        divoom_radio_more();
    }
    if (focus >= count)
    {
        focus = count ? count - 1 : 0;
    }
    if (category || ctrl->pending_focus < count || ctrl->view.count >= ctrl->view.total)
    {
        ctrl->pending_focus = -1;
    }
    memory->focus[key] = focus;
    if (!category && ctrl->append_move && focus + 1 < count)
    {
        void *next = divoom_menu_direct_ctrl_get(ctrl->direct,
            divoom_menu_direct_ctrl_get_obj(ctrl->direct, memory->column[memory->tab], focus), DIVOOM_MENU_DIRECT_DOWN);
        if (next)
        {
            focus = divoom_menu_direct_ctrl_get_pos(ctrl->direct, next) / 2;
            memory->focus[key] = focus;
        }
        ctrl->append_move = 0;
    }
    start = ctrl->window;
    if (focus < start)
    {
        start = focus;
    }
    if (focus >= start + DIVOOM_RADIO_MENU_ROWS)
    {
        start = focus - DIVOOM_RADIO_MENU_ROWS + 1;
    }
    if (ctrl->restore_scroll)
    {
        start = memory->scroll[key] / stride;
        if (focus < start || focus >= start + DIVOOM_RADIO_MENU_ROWS)
        {
            start = focus > 3 ? focus - 3 : 0;
        }
    }
    ctrl->window = start;
    visible = count - start;
    if (visible < 0)
    {
        visible = 0;
    }
    if (visible > DIVOOM_RADIO_MENU_ROWS)
    {
        visible = DIVOOM_RADIO_MENU_ROWS;
    }
    for (int32 i = 0; i < DIVOOM_RADIO_MENU_NAV_MAX; i++)
    {
        divoom_menu_direct_ctrl_update(ctrl->direct, i < count ? &ctrl->tokens[2 * i] : NULL, 0, i);
        divoom_menu_direct_ctrl_update(ctrl->direct, i < count && !category ? &ctrl->tokens[2 * i + 1] : NULL, 1, i);
    }
    if (!category)
    {
        for (int32 i = 0; i < visible; i++)
        {
            indices[i] = ctrl->map[start + i];
        }
        if (!divoom_radio_browse_rows_copy(ctrl->view.revision, indices, visible, ctrl->visible))
        {
            ctrl->dirty = 1;
            return;
        }
    }
    lv_obj_set_pos(ctrl->viewport, 4, category ? 0 : memory->tab == 0 ? 108 : 56);
    lv_obj_set_height(ctrl->viewport, category ? 400 : memory->tab == 0 ? 248 : 300);
    lv_obj_set_pos(ctrl->extent, 0, count ? (count - 1) * stride + 71 : 0);
    for (int32 i = 0; i < DIVOOM_RADIO_MENU_ROWS; i++)
    {
        uint8 focused = i < visible && start + i == focus && (category || memory->pane == DIVOOM_RADIO_MENU_LIST);
        uint8 right = !category && memory->column[memory->tab];
        lv_obj_add_flag(ctrl->rows[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ctrl->favorites[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(ctrl->rows[i], LV_STATE_FOCUSED);
        lv_obj_clear_state(ctrl->favorites[i], LV_STATE_FOCUSED);
        if (i >= visible)
        {
            continue;
        }
        lv_obj_clear_flag(ctrl->rows[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ctrl->checks[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(ctrl->rows[i], 0, (start + i) * stride);
        lv_obj_set_width(ctrl->rows[i], category ? 472 : 412);
        if (focused)
        {
            lv_obj_add_state(right ? ctrl->favorites[i] : ctrl->rows[i], LV_STATE_FOCUSED);
        }
        if (category)
        {
            uint8 selected = divoom_radio_menu_category_selected(ctrl, start + i);
            divoom_radio_menu_card(ctrl->rows[i], focused);
            if (!focused)
            {
                lv_obj_set_style_bg_color(ctrl->rows[i], lv_color_hex(0xe9e5df), 0);
            }
            if (selected)
            {
                lv_obj_clear_flag(ctrl->checks[i], LV_OBJ_FLAG_HIDDEN);
            }
            divoom_radio_menu_category_name(ctrl, start + i, text, sizeof(text));
            lv_obj_add_flag(ctrl->details[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ctrl->icons[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(ctrl->arrows[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_text_font(ctrl->names[i], divoom_menu_control_get_font_info(24, ctrl->menu_id), 0);
            lv_obj_set_align(ctrl->names[i], LV_ALIGN_TOP_LEFT);
            /* 产品字体含上沿留白；按真源字形包围框对齐，而非把行高当成字形高度。 */
            lv_obj_set_pos(ctrl->names[i], 20, 16);
            lv_obj_set_width(ctrl->names[i], selected ? 376 : 404);
        }
        else
        {
            divoom_radio_station *station = &ctrl->visible[i];
            uint8 selected = indices[i] == ctrl->view.selected;
            divoom_radio_menu_card(ctrl->rows[i], focused && !right);
            snprintf(text, sizeof(text), "%s", station->name);
            lv_obj_set_style_text_font(ctrl->names[i], divoom_menu_control_get_font_info(20, ctrl->menu_id), 0);
            lv_obj_set_width(ctrl->names[i], 348);
            lv_obj_set_pos(ctrl->details[i], 42, 34);
            const char *track = selected && ctrl->view.live_track[0] && !strstr(ctrl->view.live_track, "://") ?
                ctrl->view.live_track : station->track;
            if (track[0] && !strstr(track, "://"))
            {
                lv_obj_set_align(ctrl->names[i], LV_ALIGN_TOP_LEFT);
                lv_obj_set_pos(ctrl->names[i], 42, 5);
                lv_label_set_text(ctrl->details[i], track);
                lv_obj_clear_flag(ctrl->details[i], LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                /* 没有节目时只居中显示台名，不占次行放占位文案或格式信息。 */
                lv_label_set_text(ctrl->details[i], "");
                lv_obj_add_flag(ctrl->details[i], LV_OBJ_FLAG_HIDDEN);
                lv_obj_align(ctrl->names[i], LV_ALIGN_LEFT_MID, 42, 0);
            }
            lv_obj_clear_flag(ctrl->icons[i], LV_OBJ_FLAG_HIDDEN);
            divoom_menu_control_lv_img_set_src(ctrl->icons[i], selected ?
                (ctrl->view.play == DIVOOM_RADIO_PLAYING ? "91/source_transport_pause.png" : "91/source_transport_play.png") : "23/at_tf_music_note.png");
            /* 既有播放图含留白；让可见字形和24px音符区域对齐，不生成缩放后的新文件。 */
            lv_img_set_pivot(ctrl->icons[i], 0, 0);
            lv_img_set_zoom(ctrl->icons[i], selected ? 256 : 171);
            lv_obj_set_pos(ctrl->icons[i], selected ? 7 : 12, selected ? 20 : 22);
            lv_obj_add_flag(ctrl->arrows[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(ctrl->favorites[i], 416, (start + i) * stride);
            lv_obj_clear_flag(ctrl->favorites[i], LV_OBJ_FLAG_HIDDEN);
            divoom_radio_menu_card(ctrl->favorites[i], focused && right);
            divoom_menu_control_lv_img_set_src(ctrl->hearts[i], station->favorite ? "102/face_market_favorited.png" : "102/face_market_favorite.png");
        }
        lv_label_set_text(ctrl->names[i], text);
        lv_label_set_long_mode(ctrl->names[i], focused ? LV_LABEL_LONG_SCROLL_CIRCULAR : LV_LABEL_LONG_CLIP);
    }
    lv_obj_add_flag(ctrl->empty, LV_OBJ_FLAG_HIDDEN);
    if (!loading && divoom_radio_menu_list_failed(ctrl, category))
    {
        divoom_radio_menu_fail_set(ctrl, 1);
        if (!ctrl->fail_panel)
        {
            lv_label_set_text(ctrl->empty, divoom_menu_control_get_disp_str("NetworkConnectFail"));
            lv_obj_clear_flag(ctrl->empty, LV_OBJ_FLAG_HIDDEN);
        }
    }
    else
    {
        divoom_radio_menu_fail_set(ctrl, 0);
        if (!loading && (!count || (category && !ctrl->china && memory->parent < 0 &&
            browse.genre_load != DIVOOM_RADIO_READY)))
        {
            lv_label_set_text(ctrl->empty, divoom_radio_menu_get_text()->empty);
            lv_obj_clear_flag(ctrl->empty, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (category)
    {
        lv_obj_add_flag(ctrl->dock, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_clear_flag(ctrl->dock, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ctrl->dock_label, ctrl->view.selected_name[0] ? ctrl->view.selected_name : divoom_menu_control_get_disp_str("UnknownMusicName"));
        if (ctrl->favorite_error != ctrl->view.favorite_error)
        {
            ctrl->favorite_error = ctrl->view.favorite_error;
            lv_label_set_text(ctrl->dock_label, divoom_menu_control_get_disp_str("Failed"));
        }
        divoom_menu_control_lv_img_set_src(ctrl->dock_icon, ctrl->view.play == DIVOOM_RADIO_PLAYING ? "91/source_transport_pause.png" : "91/source_transport_play.png");
        if (count && !ctrl->view.more_error && focus >= count - 3 && ctrl->view.count < ctrl->view.total)
        {
            divoom_radio_more();
        }
        uint32 hint = count && memory->pane == DIVOOM_RADIO_MENU_LIST ? ctrl->view.items[ctrl->map[focus]].id : 0;
        if (hint != ctrl->hinted)
        {
            ctrl->hinted = hint;
            divoom_radio_hint(count && hint ? ctrl->map[focus] : -1);
        }
    }
    lv_obj_update_layout(ctrl->viewport);
    if (ctrl->restore_scroll)
    {
        lv_obj_scroll_to_y(ctrl->viewport, memory->scroll[key], LV_ANIM_OFF);
        ctrl->restore_scroll = 0;
    }
    if (count && (category || memory->pane == DIVOOM_RADIO_MENU_LIST))
    {
        lv_obj_scroll_to_view(!category && memory->column[memory->tab] ? ctrl->favorites[focus - start] : ctrl->rows[focus - start], LV_ANIM_OFF);
    }
    ctrl->rendered_key = key;
}

void *divoom_radio_menu_create(lv_obj_t *content, lv_obj_t *header, int32 menu_id, divoom_radio_menu_memory *memory)
{
    static const char *keys[] = {"NetRadioTabList", "NetRadioTabFavourite", "NetRadioTabRecent"};
    static const int16 tab_x[] = {4, 163, 321};
    divoom_radio_menu_ctrl *ctrl = divoom_system_malloc(sizeof(*ctrl));
    if (!ctrl)
    {
        return NULL;
    }
    ctrl->memory = memory;
    ctrl->rendered_key = -1;
    ctrl->pending_focus = -1;
    ctrl->menu_id = menu_id;
    ctrl->china = divoom_main_get_server_type() == 0;
    ctrl->dirty = 1;
    if (!memory->initialized)
    {
        memory->parent = -1;
        memory->filter.kind = DIVOOM_RADIO_BROWSE_AUTO;
        memory->initialized = 1;
    }
    ctrl->direct = divoom_menu_direct_ctrl_create(2, DIVOOM_RADIO_MENU_NAV_MAX);
    ctrl->tab_direct = divoom_menu_direct_ctrl_create(DIVOOM_RADIO_MENU_TAB_COUNT, 1);
    if (!ctrl->direct || !ctrl->tab_direct)
    {
        divoom_menu_direct_ctrl_destory(ctrl->direct);
        divoom_menu_direct_ctrl_destory(ctrl->tab_direct);
        divoom_system_free(ctrl);
        return NULL;
    }
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    ctrl->root = lv_obj_create(content);
    divoom_radio_menu_container(ctrl->root);
    lv_obj_set_pos(ctrl->root, 0, 8);
    lv_obj_set_size(ctrl->root, 480, 408);
    divoom_radio_menu_create_chrome(ctrl, header);
    int32 lang = divoom_main_set_lang_flag(-1);
    /* 真源中日韩用 24 px，其他语言用 16 px。焦点使用同字号的系统粗体。 */
    int32 font_size = (lang == -1 || lang == DIVOOM_LVGL_MENU_LANG_SC || lang == DIVOOM_LVGL_MENU_LANG_TC ||
        lang == DIVOOM_LVGL_MENU_LANG_JP || lang == DIVOOM_LVGL_MENU_LANG_Korean) ? 24 : 16;
    const lv_font_t *focus_font = divoom_menu_control_get_font_info_style(DIVOOM_SYSTEM_FONT_MENU_ID,
        font_size, FT_FONT_STYLE_BOLD, menu_id);
    for (int32 i = 0; i < DIVOOM_RADIO_MENU_TAB_COUNT; i++)
    {
        ctrl->tabs[i] = lv_btn_create(ctrl->root);
        divoom_radio_menu_container(ctrl->tabs[i]);
        lv_obj_set_pos(ctrl->tabs[i], tab_x[i], 0);
        lv_obj_set_size(ctrl->tabs[i], 154 + (i != 1), 46);
        lv_obj_add_event_cb(ctrl->tabs[i], divoom_radio_menu_draw_tab, LV_EVENT_DRAW_MAIN, NULL);
        lv_obj_t *label = divoom_radio_menu_label(ctrl, ctrl->tabs[i], font_size, 0, 0, 154 + (i != 1));
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(label, focus_font, LV_STATE_USER_1);
        lv_label_set_text(label, divoom_menu_control_get_disp_str(keys[i]));
        /* 24 px 字体的字面基线及末项取整按真源居中。 */
        lv_obj_align(label, LV_ALIGN_CENTER, font_size == 24 && i == 2 ? 1 : 0,
            font_size == 24 ? -3 : -2);
        divoom_menu_direct_ctrl_update(ctrl->tab_direct, ctrl->tabs[i], i, 0);
        ctrl->underlines[i] = lv_obj_create(ctrl->root);
        divoom_radio_menu_container(ctrl->underlines[i]);
        lv_obj_set_pos(ctrl->underlines[i], tab_x[i], 42);
        lv_obj_set_size(ctrl->underlines[i], 154 + (i != 1), 4);
        lv_obj_set_style_bg_opa(ctrl->underlines[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(ctrl->underlines[i], 2, 0);
    }
    ctrl->viewport = lv_obj_create(ctrl->root);
    divoom_radio_menu_container(ctrl->viewport);
    lv_obj_add_flag(ctrl->viewport, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(ctrl->viewport, 472, 300);
    lv_obj_set_scroll_dir(ctrl->viewport, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(ctrl->viewport, LV_SCROLLBAR_MODE_OFF);
    ctrl->extent = lv_obj_create(ctrl->viewport);
    divoom_radio_menu_container(ctrl->extent);
    lv_obj_set_size(ctrl->extent, 1, 1);
    for (int32 i = 0; i < DIVOOM_RADIO_MENU_ROWS; i++)
    {
        ctrl->rows[i] = lv_btn_create(ctrl->viewport);
        divoom_radio_menu_container(ctrl->rows[i]);
        lv_obj_set_size(ctrl->rows[i], 412, 72);
        ctrl->icons[i] = lv_img_create(ctrl->rows[i]);
        lv_obj_set_pos(ctrl->icons[i], 14, 24);
        ctrl->names[i] = divoom_radio_menu_label(ctrl, ctrl->rows[i], 20, 56, 12, 332);
        ctrl->details[i] = divoom_radio_menu_label(ctrl, ctrl->rows[i], 16, 44, 36, 348);
        ctrl->arrows[i] = lv_img_create(ctrl->rows[i]);
        divoom_menu_control_lv_img_set_src(ctrl->arrows[i], "23/at_sublevel_chevron.png");
        lv_obj_set_pos(ctrl->arrows[i], 436, 26);
        ctrl->checks[i] = lv_img_create(ctrl->rows[i]);
        divoom_menu_control_lv_img_set_src(ctrl->checks[i], "23/at_repeat_check.png");
        lv_obj_align(ctrl->checks[i], LV_ALIGN_RIGHT_MID, -58, 0);
        ctrl->favorites[i] = lv_btn_create(ctrl->viewport);
        divoom_radio_menu_container(ctrl->favorites[i]);
        lv_obj_set_size(ctrl->favorites[i], 56, 72);
        ctrl->hearts[i] = lv_img_create(ctrl->favorites[i]);
        divoom_menu_control_lv_img_set_src(ctrl->hearts[i], "102/face_market_favorite.png");
        lv_obj_center(ctrl->hearts[i]);
    }
    ctrl->empty = divoom_radio_menu_label(ctrl, ctrl->root, 28, 24, 180, 432);
    lv_obj_set_style_text_align(ctrl->empty, LV_TEXT_ALIGN_CENTER, 0);
    divoom_radio_menu_notice_create(ctrl);
    ctrl->dock = lv_obj_create(ctrl->root);
    divoom_radio_menu_container(ctrl->dock);
    lv_obj_set_pos(ctrl->dock, 4, 364);
    lv_obj_set_size(ctrl->dock, 472, 36);
    lv_obj_set_style_bg_color(ctrl->dock, lv_color_hex(0x243746), 0);
    lv_obj_set_style_bg_opa(ctrl->dock, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctrl->dock, 6, 0);
    ctrl->dock_icon = lv_img_create(ctrl->dock);
    lv_obj_set_pos(ctrl->dock_icon, 3, 4);
    lv_obj_set_style_img_recolor(ctrl->dock_icon, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_img_recolor_opa(ctrl->dock_icon, LV_OPA_COVER, 0);
    ctrl->dock_label = divoom_radio_menu_label(ctrl, ctrl->dock, 20, 42, 1, 420);
    lv_obj_set_style_text_color(ctrl->dock_label, lv_color_hex(0xffffff), 0);
    /* 复用LVGL spinner，不新增位图；面板不获焦、不拦按键，允许连接中取消和返回。 */
    divoom_radio_start();
    divoom_radio_set_menu_active(1);
    divoom_radio_menu_request(ctrl);
    divoom_radio_menu_refresh(ctrl);
    return ctrl;
}

void divoom_radio_menu_destroy(void *handle)
{
    divoom_radio_menu_ctrl *ctrl = handle;
    if (ctrl)
    {
        divoom_radio_menu_save_scroll(ctrl);
        divoom_lvgl_top_radio_wait_set_catalog(0, 0, 0);
        divoom_radio_set_menu_active(0);
        divoom_menu_direct_ctrl_destory(ctrl->direct);
        divoom_menu_direct_ctrl_destory(ctrl->tab_direct);
        lv_obj_del(ctrl->info_button);
        if (ctrl->title)
        {
            lv_obj_set_width(ctrl->title, LV_SIZE_CONTENT);
        }
        lv_obj_del(ctrl->root);
        divoom_system_free(ctrl);
    }
}

/* 视图刷新由100ms timer负责；切换请求后立即到来的按键不能操作上一列表的行映射。 */
static uint32 divoom_radio_menu_focus_id(divoom_radio_menu_ctrl *ctrl)
{
    divoom_radio_browse_state browse;
    if (ctrl->category_open || ctrl->info_open || ctrl->memory->pane != DIVOOM_RADIO_MENU_LIST ||
        ctrl->rendered_key != ctrl->memory->tab)
    {
        return 0;
    }
    divoom_radio_browse_poll(&browse);
    if (browse.requested != ctrl->browse.requested || browse.applied != browse.requested)
    {
        return 0;
    }
    int32 focus = ctrl->memory->focus[ctrl->memory->tab];
    if (focus < 0 || focus >= ctrl->count)
    {
        return 0;
    }
    int32 index = ctrl->map[focus];
    if (index < 0 || index >= ctrl->view.count)
    {
        return 0;
    }
    return ctrl->view.items[index].id;
}

/* 页签每次完整按键只移动一项：模拟器会在松手前先报告PRESSING，松手又报告SHORT。 */
uint8 divoom_radio_menu_key_event(void *handle, uint8 key, uint8 press)
{
    divoom_radio_menu_ctrl *ctrl = handle;
    uint8 horizontal = divoom_astrootoo_ui_key_is_left(key) || divoom_astrootoo_ui_key_is_right(key) ||
        key == DIVOOM_KEY_LIGHT_LEFT || key == DIVOOM_KEY_LIGHT_RIGHT;
    if (!ctrl)
    {
        return 0;
    }
    if (key == DIVOOM_KEY_MUSIC_PLAY || key == DIVOOM_KEY_MUSIC_FAVORITE || key == DIVOOM_LVGL_KEY_FAVORITE)
    {
        if (press == DIVOOM_PRESS_SHORT || press == DIVOOM_NO_PRESS)
        {
            uint32 id = divoom_radio_menu_focus_id(ctrl);
            /* 专键控制实际音源；类目页或加载中过期行也允许暂停后台台，收藏只针对有效焦点。 */
            if (key == DIVOOM_KEY_MUSIC_PLAY)
            {
                divoom_radio_toggle_play(id);
            }
            else if (id)
            {
                divoom_radio_favorite_id(id);
            }
        }
        return 0;
    }
    if (press == DIVOOM_PRESS_UP)
    {
        ctrl->tab_nav_held = 0;
        return 0;
    }
    if (press == DIVOOM_PRESS_SHORT || press == DIVOOM_NO_PRESS)
    {
        uint8 consumed = ctrl->tab_nav_held && ctrl->tab_nav_key == key;
        ctrl->tab_nav_held = 0;
        if (consumed)
        {
            return 0;
        }
    }
    else if (press == DIVOOM_PRESSING || press == DIVOOM_PRESS_LONG)
    {
        if (horizontal && ctrl->memory->pane == DIVOOM_RADIO_MENU_TABS)
        {
            if (ctrl->tab_nav_held && ctrl->tab_nav_key == key)
            {
                return 0;
            }
            ctrl->tab_nav_key = key;
            ctrl->tab_nav_held = 1;
        }
        else if (!divoom_astrootoo_ui_key_is_up(key) && !divoom_astrootoo_ui_key_is_down(key) && !horizontal)
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
    return divoom_radio_menu_key(handle, key);
}

uint8 divoom_radio_menu_key(void *handle, uint8 key)
{
    divoom_radio_menu_ctrl *ctrl = handle;
    if (!ctrl)
    {
        return divoom_astrootoo_ui_key_is_esc(key);
    }
    divoom_radio_menu_memory *memory = ctrl->memory;
    /* 用户新按键覆盖尚未补齐的恢复焦点；不能用旧焦点把新导航拉回去。 */
    ctrl->pending_focus = -1;
    int32 view_key = divoom_radio_menu_view_key(ctrl);
    int32 focus = memory->focus[view_key];
    uint8 category = ctrl->category_open;
    if (key == DIVOOM_KEY_LIGHT_LEFT)
    {
        key = DIVOOM_LVGL_KEY_LEFT;
    }
    else if (key == DIVOOM_KEY_LIGHT_RIGHT)
    {
        key = DIVOOM_LVGL_KEY_RIGHT;
    }
    if (category && ctrl->rendered_key != view_key &&
        !divoom_astrootoo_ui_key_is_esc(key) && !divoom_astrootoo_ui_key_is_left(key))
    {
        return 0;
    }
    if (ctrl->info_open)
    {
        if (divoom_astrootoo_ui_key_is_nav_key(key) || divoom_astrootoo_ui_key_is_enter(key) ||
            divoom_astrootoo_ui_key_is_esc(key))
        {
            /* 免责声明消费此次按键，返回原页签并恢复滚动位置。 */
            ctrl->info_open = 0;
            ctrl->restore_scroll = 1;
            ctrl->dirty = 1;
        }
        return 0;
    }
    if (divoom_astrootoo_ui_key_is_esc(key) ||
        (category && memory->parent >= 0 && divoom_astrootoo_ui_key_is_left(key)))
    {
        if (category)
        {
            divoom_radio_menu_save_scroll(ctrl);
            if (memory->parent >= 0)
            {
                memory->parent = -1;
                ctrl->select_current_category = 0;
            }
            else
            {
                ctrl->category_open = 0;
                if (!divoom_radio_menu_region_matches(ctrl, &memory->filter))
                {
                    divoom_radio_menu_request(ctrl);
                }
            }
            ctrl->restore_scroll = 1;
        }
        else if (memory->pane == DIVOOM_RADIO_MENU_TABS)
        {
            return 1;
        }
        else
        {
            memory->pane = DIVOOM_RADIO_MENU_TABS;
        }
    }
    else if (!category && memory->pane == DIVOOM_RADIO_MENU_HEADER)
    {
        if (divoom_astrootoo_ui_key_is_enter(key))
        {
            divoom_radio_menu_save_scroll(ctrl);
            ctrl->info_open = 1;
        }
        else if (divoom_astrootoo_ui_key_is_down(key) || divoom_astrootoo_ui_key_is_left(key) ||
            divoom_astrootoo_ui_key_is_right(key))
        {
            memory->pane = DIVOOM_RADIO_MENU_TABS;
        }
    }
    else if (!category && memory->pane == DIVOOM_RADIO_MENU_FILTER)
    {
        if (divoom_astrootoo_ui_key_is_enter(key))
        {
            divoom_radio_menu_save_scroll(ctrl);
            ctrl->category_open = 1;
            ctrl->select_current_category = 1;
            memory->parent = -1;
            memory->focus[3] = 0;
            memory->scroll[3] = 0;
            ctrl->restore_scroll = 1;
            divoom_radio_menu_request(ctrl);
        }
        else if (divoom_astrootoo_ui_key_is_up(key))
        {
            memory->pane = DIVOOM_RADIO_MENU_TABS;
        }
        else if (divoom_astrootoo_ui_key_is_down(key) && ctrl->count)
        {
            memory->pane = DIVOOM_RADIO_MENU_LIST;
            memory->column[0] = 0;
        }
    }
    else if (!category && memory->pane == DIVOOM_RADIO_MENU_TABS)
    {
        if (divoom_astrootoo_ui_key_is_left(key) || divoom_astrootoo_ui_key_is_right(key))
        {
            void *next = divoom_menu_direct_ctrl_get(ctrl->tab_direct, ctrl->tabs[memory->tab],
                divoom_astrootoo_ui_key_is_left(key) ? DIVOOM_MENU_DIRECT_LEFT : DIVOOM_MENU_DIRECT_RIGHT);
            if (next)
            {
                divoom_radio_menu_save_scroll(ctrl);
                memory->tab = divoom_menu_direct_ctrl_get_pos(ctrl->tab_direct, next);
                ctrl->append_move = 0;
                ctrl->restore_scroll = 1;
                divoom_radio_menu_request(ctrl);
            }
        }
        else if (divoom_astrootoo_ui_key_is_up(key))
        {
            memory->pane = DIVOOM_RADIO_MENU_HEADER;
        }
        else if (divoom_astrootoo_ui_key_is_down(key) || divoom_astrootoo_ui_key_is_enter(key))
        {
            if (memory->tab == 0)
            {
                memory->pane = DIVOOM_RADIO_MENU_FILTER;
            }
            else if (ctrl->count)
            {
                memory->pane = DIVOOM_RADIO_MENU_LIST;
                memory->column[memory->tab] = 0;
            }
        }
    }
    else if (!category && divoom_astrootoo_ui_key_is_up(key) && (!ctrl->count || focus == 0))
    {
        memory->pane = memory->tab == 0 ? DIVOOM_RADIO_MENU_FILTER : DIVOOM_RADIO_MENU_TABS;
    }
    else if (divoom_astrootoo_ui_key_is_enter(key))
    {
        if (category)
        {

            if (!ctrl->count)
            {
                divoom_radio_menu_request(ctrl);
            }
            else if (ctrl->china)
            {
                char name[96];
                divoom_radio_browse_query query = {0};
                if (!divoom_radio_browse_classify_copy(focus, &query.classify_id, name, sizeof(name)))
                {
                    return 0;
                }
                query.kind = DIVOOM_RADIO_BROWSE_DOMESTIC;
                divoom_radio_menu_apply_filter(ctrl, &query);
            }
            else if (memory->parent < 0)
            {
                divoom_radio_menu_save_scroll(ctrl);
                divoom_radio_browse_genre genre;
                if (focus == 0)
                {
                    divoom_radio_browse_query query = {0};
                    query.kind = DIVOOM_RADIO_BROWSE_TOP;
                    divoom_radio_menu_apply_filter(ctrl, &query);
                    ctrl->dirty = 1;
                    return 0;
                }
                else if (divoom_radio_browse_genre_copy(0, focus - 1, &genre))
                {
                    memory->parent = genre.id;
                    snprintf(memory->parent_name, sizeof(memory->parent_name), "%s", genre.name);
                }
                else
                {
                    return 0;
                }
                ctrl->select_current_category = 1;
                memory->focus[4] = 0;
                memory->scroll[4] = 0;
                if (!divoom_radio_browse_genre_count(memory->parent))
                {
                    divoom_radio_browse_query query = {0};
                    query.kind = DIVOOM_RADIO_BROWSE_GENRE;
                    snprintf(query.genre, sizeof(query.genre), "%s", memory->parent_name);
                    memory->parent = -1;
                    divoom_radio_menu_apply_filter(ctrl, &query);
                }
                ctrl->restore_scroll = 1;
            }
            else
            {
                divoom_radio_browse_query query = {0};
                divoom_radio_browse_genre genre;
                query.kind = DIVOOM_RADIO_BROWSE_GENRE;
                if (!focus)
                {
                    snprintf(query.genre, sizeof(query.genre), "%s", memory->parent_name);
                }
                else if (divoom_radio_browse_genre_copy(memory->parent, focus - 1, &genre))
                {
                    snprintf(query.genre, sizeof(query.genre), "%s", genre.name);
                }
                else
                {
                    return 0;
                }
                divoom_radio_menu_apply_filter(ctrl, &query);
            }
        }
        else if (focus < ctrl->count)
        {
            uint32 id = divoom_radio_menu_focus_id(ctrl);
            if (!id)
            {
                return 0;
            }
            if (memory->column[memory->tab])
            {
                divoom_radio_favorite_id(id);
            }
            else
            {
                divoom_radio_activate_id(id);
            }
        }
        else
        {
            divoom_radio_reload();
        }
    }
    else if (ctrl->count && (divoom_astrootoo_ui_key_is_up(key) || divoom_astrootoo_ui_key_is_down(key) ||
        divoom_astrootoo_ui_key_is_left(key) || divoom_astrootoo_ui_key_is_right(key)))
    {
        uint8 left = divoom_astrootoo_ui_key_is_left(key), right = divoom_astrootoo_ui_key_is_right(key);
        if (category)
        {
            ctrl->select_current_category = 0;
        }
        uint8 column = category ? 0 : memory->column[memory->tab];
        if ((left && (category || !column)) || (right && (category || column)) ||
            (category && focus == 0 && divoom_astrootoo_ui_key_is_up(key)) ||
            (divoom_astrootoo_ui_key_is_down(key) && focus >= ctrl->count - 1))
        {
            if (!category && divoom_astrootoo_ui_key_is_down(key) && ctrl->view.count < ctrl->view.total)
            {
                ctrl->append_move = 1;
                divoom_radio_more();
            }
            return 0;
        }
        int32 direct = left ? DIVOOM_MENU_DIRECT_LEFT : right ? DIVOOM_MENU_DIRECT_RIGHT :
            divoom_astrootoo_ui_key_is_up(key) ? DIVOOM_MENU_DIRECT_UP : DIVOOM_MENU_DIRECT_DOWN;
        void *next = divoom_menu_direct_ctrl_get(ctrl->direct,
            divoom_menu_direct_ctrl_get_obj(ctrl->direct, column, focus), direct);
        if (next)
        {
            int32 position = divoom_menu_direct_ctrl_get_pos(ctrl->direct, next);
            memory->focus[view_key] = position / 2;
            if (!category)
            {
                memory->column[memory->tab] = position % 2;
            }
        }
    }
    ctrl->dirty = 1;
    return 0;
}

int divoom_radio_menu_probe(void *handle, const char *name, long *out)
{
    divoom_radio_menu_ctrl *ctrl = handle;
    if (!ctrl || !out || !name)
    {
        return 0;
    }
    int32 focus = ctrl->memory->focus[divoom_radio_menu_view_key(ctrl)];
    if (!strcmp(name, "count")) {
        *out = ctrl->count;
    }
    else if (!strcmp(name, "loaded")) {
        *out = ctrl->view.count;
    }
    else if (!strcmp(name, "total")) {
        *out = ctrl->view.total;
    }
    else if (!strcmp(name, "page_requests")) {
        *out = ctrl->view.page_requests;
    }
    else if (!strcmp(name, "focus")) {
        *out = ctrl->count ? focus + 1 : 0;
    }
    else if (!strcmp(name, "tab")) {
        *out = ctrl->memory->tab;
    }
    else if (!strcmp(name, "parent")) {
        *out = ctrl->memory->parent;
    }
    else if (!strcmp(name, "classify_id")) {
        *out = ctrl->browse.active_query.classify_id;
    }
    else if (!strcmp(name, "kind")) {
        *out = ctrl->browse.active_query.kind;
    }
    else if (!strcmp(name, "column")) {
        *out = ctrl->category_open ? 0 : ctrl->memory->column[ctrl->memory->tab];
    }
    else if (!strcmp(name, "pane")) {
        *out = ctrl->memory->pane == DIVOOM_RADIO_MENU_TABS;
    }
    else if (!strcmp(name, "screen")) {
        *out = ctrl->info_open ? 2 : ctrl->category_open;
    }
    else if (!strcmp(name, "control_focus")) {
        *out = ctrl->memory->pane;
    }
    else if (!strcmp(name, "play")) {
        *out = ctrl->view.play;
    }
    else if (!strcmp(name, "selected")) {
        *out = ctrl->view.selected;
    }
    else if (!strcmp(name, "load")) {
        *out = ctrl->view.load;
    }
    else if (!strcmp(name, "favorite_error")) {
        *out = ctrl->view.favorite_error;
    }
    else if (!strcmp(name, "focus_favorite")) {
        *out = !ctrl->category_open && focus < ctrl->count ? ctrl->view.items[ctrl->map[focus]].favorite : -1;
    }
    else if (!strcmp(name, "layout") || !strcmp(name, "focus_visible") || !strcmp(name, "focus_marked"))
    {
        int32 row = focus - ctrl->window;
        uint8 category = ctrl->category_open;
        uint8 right = !category && ctrl->memory->column[ctrl->memory->tab];
        *out = 0;
        if (ctrl->count && row >= 0 && row < DIVOOM_RADIO_MENU_ROWS)
        {
            /* 真正读取可见对象和资源路径，不能用固定PASS代替布局/焦点验收。 */
            lv_obj_t *obj = right ? ctrl->favorites[row] : ctrl->rows[row];
            lv_obj_update_layout(ctrl->root);
            if (!strcmp(name, "layout"))
            {
                *out = lv_obj_get_width(ctrl->viewport) == 472 &&
                    lv_obj_get_width(ctrl->rows[row]) == (category ? 472 : 412) &&
                    (category || lv_obj_get_width(ctrl->favorites[row]) == 56);
            }
            else if (!strcmp(name, "focus_visible"))
            {
                lv_area_t item, viewport;
                lv_obj_get_coords(obj, &item);
                lv_obj_get_coords(ctrl->viewport, &viewport);
                *out = (category || ctrl->memory->pane == DIVOOM_RADIO_MENU_LIST) && !lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN) &&
                    item.x1 >= viewport.x1 && item.x2 <= viewport.x2 &&
                    item.y1 >= viewport.y1 && item.y2 <= viewport.y2;
            }
            else if ((category || ctrl->memory->pane == DIVOOM_RADIO_MENU_LIST) && lv_obj_has_state(obj, LV_STATE_FOCUSED))
            {
                lv_color_t actual = lv_obj_get_style_bg_color(obj, 0);
                lv_color_t expected = lv_color_hex(0xf2b63a);
                *out = lv_obj_get_style_bg_opa(obj, 0) == LV_OPA_COVER &&
                    !memcmp(&actual, &expected, sizeof(actual)) && lv_obj_get_style_border_width(obj, 0) == 2;
            }
        }
    }
    else {
        return 0;
    }
    return 1;
}
