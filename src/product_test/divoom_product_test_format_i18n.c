#include "divoom.h"
#include "lvgl.h"
#include "divoom_lvgl9_compat.h"
#include "divoom_menu_control.h"
#include "divoom_port_fs.h"
#include "divoom_product_test_format_i18n.h"
#include "divoom_product_test_text.h"
#include "divoom_product_test_default_cjk.h"
#include "divoom_update_system_file.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdlib.h>

/* Font 28 is 20 MB. Keep only this screen's glyphs in RAM before either
 * partition is erased; never retain a FreeType file handle across formatting. */
#define PT_FORMAT_GLYPHS 1024
static const char *const s_format_keys[] = {
    "Format /userdata", "Format /usr", "Format /usr requires saved Wi-Fi",
    "Format /userdata and restart?", "Connecting saved Wi-Fi",
    "Downloading resource guide", "Formatting", "Restoring resources",
    "File operations are busy", "No saved Wi-Fi", "Wi-Fi connection timed out",
    "Resource guide download or validation failed", "Format failed",
    "Resource restore failed. Retry", "Ready", "Failed", "Restarting",
    "Cancelled", "Confirm", "Cancel", "Select", "FormatSDConfig", "Progress",
    "Check System File",
    "Checking file system", "System Partition", "User Partition", "Checking FAT", "Checking directories", "Checking cluster ownership", "Verifying file system", "Recovery continues automatically", "Storage recovery failed", "Starting repair", "Checking files", "Check passed", "Press any key to return", "Press any key to reboot", "Waiting for storage", "Error files: %d", "Automatic retries: %d", "FAT and directories OK on both partitions",
    "Check results",
    "Repair system files?",
    "Repair partitions?",
    "Start repair",
    "Format User Partition. Erase all user data.",
    "Format System Partition. Erase and redownload all system files.",
    "Remove %d damaged files.",
    "Redownload %d system files.",
    "Continue checking system files to determine downloads.",
    "Repair requires Wi-Fi.",
    "Retrying failed files", "Cleaning failed files", "Completed with skips", "Skipped files: %d",
    "Recovery stopped", "Operation timed out", "Retry limit reached",
};
static const char *const s_format_locales[DIVOOM_LVGL_MENU_LANG_END] = {
    "en", "sc", "tc", "ja", "de", "th", "ru", "ko", "vi", "fr",
    "tr", "es", "pt", "in", "it", "uk", "sw", "da", "pl",
};
/* Older installed language files predate these factory confirmation keys.
 * Keep the existing dialog and wording; snapshot the same translations in RAM
 * even when those tags are absent. Never update /usr resources to fix a label. */
static const char *const s_format_confirm_keys[] = {
    "Format /userdata", "Format /usr", "Format /usr requires saved Wi-Fi", "Format /userdata and restart?", "Downloading resource guide", "Restoring resources", "Restarting",
    "Check System File",
};
static const char *const s_format_confirm_texts[DIVOOM_LVGL_MENU_LANG_END][8] = {
    { /* en */
        "Format User Partition",
        "Format System Partition",
        "Formatting the System Partition requires saved Wi-Fi",
        "Format the User Partition and restart?",
        "Downloading resource guide",
        "Restoring resources",
        "Restarting",
        "Check System File",
    },
    { /* sc */
        "格式化用户分区",
        "格式化系统分区",
        "格式化系统分区需要已保存的无线网络",
        "格式化用户分区并重启？",
        "下载资源指引",
        "正在恢复资源",
        "正在重启",
        "自检系统文件",
    },
    { /* tc */
        "格式化用戶分區",
        "格式化系統分區",
        "格式化系統分區需要已儲存的無線網路",
        "格式化用戶分區並重新啟動？",
        "下載資源指引",
        "正在還原資源",
        "正在重新啟動",
        "自檢系統檔案",
    },
    { /* ja */
        "ユーザーパーティションを初期化",
        "システムパーティションを初期化",
        "システムパーティションの初期化には保存済み Wi-Fi が必要です",
        "ユーザーパーティションを初期化して再起動しますか？",
        "リソース一覧をダウンロード中",
        "リソースを復元中",
        "再起動中",
        "システムファイルの自己診断",
    },
    { /* de */
        "Benutzerpartition formatieren",
        "Systempartition formatieren",
        "Die Systempartition benötigt gespeichertes WLAN",
        "Benutzerpartition formatieren und neu starten?",
        "Ressourcenliste herunterladen",
        "Ressourcen wiederherstellen",
        "Neustart läuft",
        "Systemdateien prüfen",
    },
    { /* th */
        "ฟอร์แมตพาร์ทิชันผู้ใช้",
        "ฟอร์แมตพาร์ทิชันระบบ",
        "การฟอร์แมตพาร์ทิชันระบบต้องใช้ Wi-Fi ที่บันทึกไว้",
        "ฟอร์แมตพาร์ทิชันผู้ใช้แล้วเริ่มใหม่หรือไม่?",
        "กำลังดาวน์โหลดรายการทรัพยากร",
        "กำลังกู้คืนทรัพยากร",
        "กำลังเริ่มใหม่",
        "ตรวจสอบไฟล์ระบบ",
    },
    { /* ru */
        "Форматировать пользовательский раздел",
        "Форматировать системный раздел",
        "Для системного раздела нужна сохранённая сеть Wi-Fi",
        "Форматировать пользовательский раздел и перезапустить?",
        "Загрузка списка ресурсов",
        "Восстановление ресурсов",
        "Перезапуск",
        "Проверка системных файлов",
    },
    { /* ko */
        "사용자 파티션 포맷",
        "시스템 파티션 포맷",
        "시스템 파티션 포맷에는 저장된 Wi-Fi가 필요합니다",
        "사용자 파티션을 포맷하고 재시작할까요?",
        "리소스 목록 다운로드 중",
        "리소스 복원 중",
        "재시작 중",
        "시스템 파일 자가 진단",
    },
    { /* vi */
        "Định dạng phân vùng người dùng",
        "Định dạng phân vùng hệ thống",
        "Phân vùng hệ thống cần Wi-Fi đã lưu",
        "Định dạng phân vùng người dùng và khởi động lại?",
        "Đang tải danh sách tài nguyên",
        "Đang khôi phục tài nguyên",
        "Đang khởi động lại",
        "Tự kiểm tra tệp hệ thống",
    },
    { /* fr */
        "Formater la partition utilisateur",
        "Formater la partition système",
        "La partition système nécessite un Wi-Fi enregistré",
        "Formater la partition utilisateur et redémarrer ?",
        "Téléchargement de la liste des ressources",
        "Restauration des ressources",
        "Redémarrage en cours",
        "Vérifier les fichiers système",
    },
    { /* tr */
        "Kullanıcı bölümünü biçimlendir",
        "Sistem bölümünü biçimlendir",
        "Sistem bölümü için kayıtlı Wi-Fi gerekir",
        "Kullanıcı bölümü biçimlendirilsin ve yeniden başlatılsın mı?",
        "Kaynak listesi indiriliyor",
        "Kaynaklar geri yükleniyor",
        "Yeniden başlatılıyor",
        "Sistem dosyalarını denetle",
    },
    { /* es */
        "Formatear la partición de usuario",
        "Formatear la partición del sistema",
        "La partición del sistema requiere una red Wi-Fi guardada",
        "¿Formatear la partición de usuario y reiniciar?",
        "Descargando lista de recursos",
        "Restaurando recursos",
        "Reiniciando",
        "Comprobar archivos del sistema",
    },
    { /* pt */
        "Formatar a partição do utilizador",
        "Formatar a partição do sistema",
        "A partição do sistema requer uma rede Wi-Fi guardada",
        "Formatar a partição do utilizador e reiniciar?",
        "A transferir lista de recursos",
        "A restaurar recursos",
        "A reiniciar",
        "Verificar arquivos do sistema",
    },
    { /* in */
        "Format partisi pengguna",
        "Format partisi sistem",
        "Partisi sistem memerlukan Wi-Fi tersimpan",
        "Format partisi pengguna dan mulai ulang?",
        "Mengunduh daftar sumber daya",
        "Memulihkan sumber daya",
        "Memulai ulang",
        "Periksa file sistem",
    },
    { /* it */
        "Formatta la partizione utente",
        "Formatta la partizione di sistema",
        "La partizione di sistema richiede una rete Wi-Fi salvata",
        "Formattare la partizione utente e riavviare?",
        "Download elenco risorse",
        "Ripristino risorse",
        "Riavvio",
        "Controlla i file di sistema",
    },
    { /* uk */
        "Форматувати розділ користувача",
        "Форматувати системний розділ",
        "Для системного розділу потрібна збережена мережа Wi-Fi",
        "Форматувати розділ користувача і перезапустити?",
        "Завантаження списку ресурсів",
        "Відновлення ресурсів",
        "Перезапуск",
        "Перевірка системних файлів",
    },
    { /* sw */
        "Formatera användarpartitionen",
        "Formatera systempartitionen",
        "Systempartitionen kräver ett sparat Wi-Fi-nätverk",
        "Formatera användarpartitionen och starta om?",
        "Inapakua orodha ya rasilimali",
        "Inarejesha rasilimali",
        "Inaanza upya",
        "Kontrollera systemfiler",
    },
    { /* da */
        "Formatér brugerpartitionen",
        "Formatér systempartitionen",
        "Systempartitionen kræver gemt Wi-Fi",
        "Formatér brugerpartitionen og genstart?",
        "Henter ressourceliste",
        "Gendanner ressourcer",
        "Genstarter",
        "Kontrollér systemfiler",
    },
    { /* pl */
        "Formatuj partycję użytkownika",
        "Formatuj partycję systemową",
        "Partycja systemowa wymaga zapisanej sieci Wi-Fi",
        "Sformatować partycję użytkownika i uruchomić ponownie?",
        "Pobieranie listy zasobów",
        "Przywracanie zasobów",
        "Ponowne uruchamianie",
        "Sprawdź pliki systemowe",
    },
};

static const char *const s_integrity_keys[] = {
    "Checking file system",
    "System Partition",
    "User Partition",
    "Checking FAT",
    "Checking directories",
    "Checking cluster ownership",
    "Verifying file system",
    "Recovery continues automatically",
    "Storage recovery failed",
    "Starting repair",
    "Checking files",
    "Check passed",
    "Press any key to return",
    "Press any key to reboot",
    "Waiting for storage",
    "Error files: %d",
    "Automatic retries: %d",
    "FAT and directories OK on both partitions"
};
static const char *const s_integrity_texts[DIVOOM_LVGL_MENU_LANG_END][sizeof(s_integrity_keys) / sizeof(s_integrity_keys[0])] = {
    {"Checking file system", "System Partition", "User Partition", "Checking FAT", "Checking directories", "Checking cluster ownership", "Verifying file system", "Recovery continues automatically", "Storage recovery failed", "Starting repair", "Checking files", "Check passed", "Press any key to return", "Press any key to reboot", "Waiting for storage", "Error files: %d", "Retries: %d", "FAT and directories OK on both partitions"},
    {"正在检查文件系统", "系统分区", "用户分区", "正在检查分配表", "正在检查目录", "正在检查簇归属", "正在复核文件系统", "将自动继续恢复", "存储恢复失败", "正在启动修复", "正在检查文件", "检查通过", "按任意键返回", "按任意键重启", "正在等待存储就绪", "错误文件：%d", "自动重试：%d", "双盘 FAT 和目录结构检查通过"},
    {"正在檢查檔案系統", "系統分區", "用戶分區", "正在檢查配置表", "正在檢查目錄", "正在檢查叢集歸屬", "正在複核檔案系統", "將自動繼續還原", "儲存還原失敗", "正在啟動修復", "正在檢查檔案", "檢查通過", "按任意鍵返回", "按任意鍵重新啟動", "正在等待儲存就緒", "錯誤檔案：%d", "自動重試：%d", "雙碟 FAT 和目錄結構檢查通過"},
    {"ファイルシステムを確認中", "システムパーティション", "ユーザーパーティション", "FATを確認中", "ディレクトリを確認中", "クラスタの割り当てを確認中", "ファイルシステムを再確認中", "復旧を自動で続行します", "ストレージの復旧に失敗", "修復を開始しています", "ファイルを確認中", "チェック完了", "任意のキーで戻る", "任意のキーで再起動", "ストレージの準備を待っています", "エラーファイル：%d", "自動再試行：%d", "両パーティションの FAT とディレクトリは正常"},
    {"Dateisystem wird geprüft", "Systempartition", "Benutzerpartition", "FAT wird geprüft", "Verzeichnisse werden geprüft", "Clusterzuordnung wird geprüft", "Dateisystem wird erneut geprüft", "Wiederherstellung wird automatisch fortgesetzt", "Speicherwiederherstellung fehlgeschlagen", "Reparatur wird gestartet", "Dateien werden geprüft", "Prüfung bestanden", "Beliebige Taste zum Zurückkehren", "Beliebige Taste für Neustart", "Warten auf Speicher", "Dateifehler: %d", "Versuche: %d", "FAT und Verzeichnisse beider Partitionen OK"},
    {"กำลังตรวจสอบระบบไฟล์", "พาร์ทิชันระบบ", "พาร์ทิชันผู้ใช้", "กำลังตรวจสอบ FAT", "กำลังตรวจสอบไดเรกทอรี", "กำลังตรวจสอบการจัดสรรคลัสเตอร์", "กำลังตรวจสอบระบบไฟล์ซ้ำ", "จะกู้คืนต่อโดยอัตโนมัติ", "กู้คืนพื้นที่เก็บข้อมูลไม่สำเร็จ", "กำลังเริ่มซ่อมแซม", "กำลังตรวจสอบไฟล์", "ตรวจสอบผ่าน", "กดปุ่มใดก็ได้เพื่อกลับ", "กดปุ่มใดก็ได้เพื่อเริ่มใหม่", "กำลังรอพื้นที่เก็บข้อมูลพร้อม", "ไฟล์ผิดพลาด: %d", "ลองใหม่อัตโนมัติ: %d", "FAT และไดเรกทอรีของทั้งสองพาร์ทิชันปกติ"},
    {"Проверка файловой системы", "Системный раздел", "Пользовательский раздел", "Проверка FAT", "Проверка каталогов", "Проверка распределения кластеров", "Повторная проверка файловой системы", "Восстановление продолжится автоматически", "Не удалось восстановить хранилище", "Запуск восстановления", "Проверка файлов", "Проверка пройдена", "Любая клавиша для возврата", "Любая клавиша для перезапуска", "Ожидание готовности хранилища", "Ошибок: %d", "Повторы: %d", "FAT и каталоги обоих разделов в порядке"},
    {"파일 시스템 확인 중", "시스템 파티션", "사용자 파티션", "FAT 확인 중", "디렉터리 확인 중", "클러스터 할당 확인 중", "파일 시스템 재확인 중", "복구가 자동으로 계속됩니다", "저장소 복구 실패", "복구 시작 중", "파일 확인 중", "검사 통과", "아무 키나 눌러 돌아가기", "아무 키나 눌러 재시작", "저장소 준비 대기 중", "오류 파일: %d", "자동 재시도: %d", "두 파티션의 FAT 및 디렉터리 정상"},
    {"Đang kiểm tra hệ thống tệp", "Phân vùng hệ thống", "Phân vùng người dùng", "Đang kiểm tra FAT", "Đang kiểm tra thư mục", "Đang kiểm tra phân bổ cụm", "Đang kiểm tra lại hệ thống tệp", "Khôi phục sẽ tự động tiếp tục", "Khôi phục bộ nhớ thất bại", "Đang bắt đầu sửa chữa", "Đang kiểm tra tệp", "Kiểm tra đạt", "Nhấn phím bất kỳ để trở lại", "Nhấn phím bất kỳ để khởi động lại", "Đang chờ bộ nhớ sẵn sàng", "Tệp lỗi: %d", "Thử lại: %d", "FAT và thư mục của cả hai phân vùng đều tốt"},
    {"Vérification du système de fichiers", "Partition système", "Partition utilisateur", "Vérification de la FAT", "Vérification des dossiers", "Vérification de l’allocation des clusters", "Nouvelle vérification du système de fichiers", "La restauration se poursuit automatiquement", "Échec de la restauration du stockage", "Démarrage de la réparation", "Vérification des fichiers", "Vérification réussie", "Appuyez sur une touche pour revenir", "Appuyez sur une touche pour redémarrer", "En attente du stockage", "Erreurs : %d", "Essais : %d", "FAT et dossiers des deux partitions vérifiés"},
    {"Dosya sistemi denetleniyor", "Sistem bölümü", "Kullanıcı bölümü", "FAT denetleniyor", "Dizinler denetleniyor", "Küme tahsisi denetleniyor", "Dosya sistemi yeniden denetleniyor", "Kurtarma otomatik olarak sürecek", "Depolama kurtarma başarısız", "Onarım başlatılıyor", "Dosyalar denetleniyor", "Denetim başarılı", "Dönmek için bir tuşa basın", "Yeniden başlatmak için bir tuşa basın", "Depolama bekleniyor", "Hatalar: %d", "Denemeler: %d", "İki bölümün FAT ve dizinleri sağlam"},
    {"Comprobando sistema de archivos", "Partición del sistema", "Partición de usuario", "Comprobando FAT", "Comprobando directorios", "Comprobando asignación de clústeres", "Verificando sistema de archivos", "La recuperación continuará automáticamente", "Error al recuperar el almacenamiento", "Iniciando reparación", "Comprobando archivos", "Comprobación superada", "Pulse cualquier tecla para volver", "Pulse cualquier tecla para reiniciar", "Esperando al almacenamiento", "Errores: %d", "Reintentos: %d", "FAT y directorios correctos en ambas particiones"},
    {"A verificar o sistema de ficheiros", "Partição do sistema", "Partição do utilizador", "A verificar a FAT", "A verificar diretórios", "A verificar a alocação de clusters", "A reverificar o sistema de ficheiros", "A recuperação continuará automaticamente", "Falha na recuperação do armazenamento", "A iniciar reparação", "A verificar ficheiros", "Verificação concluída", "Prima qualquer tecla para voltar", "Prima qualquer tecla para reiniciar", "À espera do armazenamento", "Erros: %d", "Tentativas: %d", "FAT e diretórios OK nas duas partições"},
    {"Memeriksa sistem file", "Partisi sistem", "Partisi pengguna", "Memeriksa FAT", "Memeriksa direktori", "Memeriksa alokasi kluster", "Memverifikasi ulang sistem file", "Pemulihan berlanjut secara otomatis", "Pemulihan penyimpanan gagal", "Memulai perbaikan", "Memeriksa file", "Pemeriksaan lulus", "Tekan tombol apa saja untuk kembali", "Tekan tombol apa saja untuk memulai ulang", "Menunggu penyimpanan siap", "File rusak: %d", "Percobaan: %d", "FAT dan direktori kedua partisi lulus"},
    {"Controllo del file system", "Partizione di sistema", "Partizione utente", "Controllo FAT", "Controllo directory", "Controllo allocazione cluster", "Nuova verifica del file system", "Il ripristino continua automaticamente", "Ripristino memoria non riuscito", "Avvio riparazione", "Controllo file", "Controllo superato", "Premi un tasto per tornare", "Premi un tasto per riavviare", "In attesa della memoria", "Errori: %d", "Tentativi: %d", "FAT e directory di entrambe le partizioni OK"},
    {"Перевірка файлової системи", "Системний розділ", "Розділ користувача", "Перевірка FAT", "Перевірка каталогів", "Перевірка розподілу кластерів", "Повторна перевірка файлової системи", "Відновлення продовжиться автоматично", "Не вдалося відновити сховище", "Запуск відновлення", "Перевірка файлів", "Перевірку пройдено", "Натисніть будь-яку клавішу для повернення", "Натисніть будь-яку клавішу для перезапуску", "Очікування готовності сховища", "Помилок: %d", "Повтори: %d", "FAT і каталоги обох розділів справні"},
    {"Kontrollerar filsystemet", "Systempartition", "Användarpartition", "Kontrollerar FAT", "Kontrollerar kataloger", "Kontrollerar klustertilldelning", "Verifierar filsystemet igen", "Återställningen fortsätter automatiskt", "Lagringen kunde inte återställas", "Startar reparation", "Kontrollerar filer", "Kontrollen godkänd", "Tryck på valfri tangent för att gå tillbaka", "Tryck på valfri tangent för att starta om", "Väntar på lagringen", "Filfel: %d", "Omförsök: %d", "FAT och kataloger på båda partitionerna OK"},
    {"Kontrollerer filsystemet", "Systempartition", "Brugerpartition", "Kontrollerer FAT", "Kontrollerer mapper", "Kontrollerer klyngetildeling", "Kontrollerer filsystemet igen", "Gendannelsen fortsætter automatisk", "Gendannelse af lager mislykkedes", "Starter reparation", "Kontrollerer filer", "Kontrol bestået", "Tryk på en tast for at gå tilbage", "Tryk på en tast for at genstarte", "Venter på lageret", "Filfejl: %d", "Forsøg: %d", "FAT og mapper på begge partitioner OK"},
    {"Sprawdzanie systemu plików", "Partycja systemowa", "Partycja użytkownika", "Sprawdzanie FAT", "Sprawdzanie katalogów", "Sprawdzanie przydziału klastrów", "Ponowne sprawdzanie systemu plików", "Odzyskiwanie będzie kontynuowane automatycznie", "Nie udało się odzyskać pamięci", "Uruchamianie naprawy", "Sprawdzanie plików", "Sprawdzanie zakończone pomyślnie", "Naciśnij dowolny klawisz, aby wrócić", "Naciśnij dowolny klawisz, aby uruchomić ponownie", "Oczekiwanie na pamięć", "Błędy: %d", "Próby: %d", "FAT i katalogi obu partycji poprawne"},
};

static const char *pt_integrity_fallback(const char *key, int lang)
{
    for (unsigned i = 0; i < sizeof(s_integrity_keys)/sizeof(s_integrity_keys[0]); ++i)
        if (!strcmp(key, s_integrity_keys[i])) return s_integrity_texts[lang][i];
    return NULL;
}

/* Indexed by FF_CHECK_STAGE_*; zero belongs to the existing CRC pass. */
const char *divoom_product_test_integrity_stage_key(unsigned stage)
{
    static const char *const keys[] = { "Checking files", "Checking FAT", "Checking directories",
        "Checking cluster ownership", "Verifying file system" };
    return keys[stage < sizeof(keys)/sizeof(keys[0]) ? stage : 0];
}

const char *divoom_product_test_integrity_text(int lang, const char *key)
{
    if (lang < 0 || lang >= DIVOOM_LVGL_MENU_LANG_END) lang = DIVOOM_LVGL_MENU_LANG_ENGLISH;
    const char *text = pt_integrity_fallback(key, lang);
    return text ? text : key;
}

/* The repair dialog must work with older installed language resources.
 * Numeric templates are fixed here so the count always matches the plan. */
static const char *pt_repair_confirm_fallback(const char *key, int lang)
{
    static const char *const texts[][3] = {
        {"Check results", "自检结果", "自檢結果"},
        {"Repair system files?", "修复系统文件？", "修復系統檔案？"},
        {"Repair partitions?", "修复分区？", "修復分區？"},
        {"Start repair", "开始修复", "開始修復"},
        {"Format User Partition. Erase all user data.", "格式化并清空用户分区。", "格式化並清空用戶分區。"},
        {"Format System Partition. Erase and redownload all system files.", "格式化系统分区，清空并重新下载全部系统文件。", "格式化系統分區，清除並重新下載全部系統檔案。"},
        {"Remove %d damaged files.", "清除 %d 个受损文件。", "清除 %d 個受損檔案。"},
        {"Redownload %d system files.", "重新下载 %d 个系统文件。", "重新下載 %d 個系統檔案。"},
        {"Continue checking system files to determine downloads.", "继续检查系统文件，再确定下载数量。", "繼續檢查系統檔案，再確定下載數量。"},
        {"Repair requires Wi-Fi.", "修复需要连接 Wi-Fi。", "修復需要連接 Wi-Fi。"},
    };
    int column = lang == DIVOOM_LVGL_MENU_LANG_SC ? 1 :
        lang == DIVOOM_LVGL_MENU_LANG_TC ? 2 : 0;
    for (unsigned i = 0; i < sizeof(texts) / sizeof(texts[0]); ++i)
        if (!strcmp(key, texts[i][0])) return texts[i][column];
    return NULL;
}

static const char *pt_restore_fallback(const char *key, int lang)
{
    static const char *const texts[DIVOOM_LVGL_MENU_LANG_END][4] = {
        {"Retrying failed files", "Cleaning failed files", "Completed with skips", "Skipped files: %d"},
        {"重试受损文件", "清理受损文件", "恢复完成，部分文件已跳过", "跳过文件：%d"},
        {"重試受損檔案", "清理受損檔案", "還原完成，部分檔案已略過", "略過檔案：%d"},
        {"失敗したファイルを再試行", "破損ファイルを削除中", "一部ファイルをスキップして完了", "スキップ：%d"},
        {"Dateien erneut versuchen", "Defekte Dateien löschen", "Mit Auslassungen beendet", "Übersprungen: %d"},
        {"ลองไฟล์ที่ล้มเหลวอีกครั้ง", "ล้างไฟล์ที่เสียหาย", "เสร็จแล้ว โดยข้ามบางไฟล์", "ไฟล์ที่ข้าม: %d"},
        {"Повтор повреждённых файлов", "Удаление повреждённых файлов", "Завершено с пропусками", "Пропущено файлов: %d"},
        {"실패한 파일 재시도", "손상된 파일 정리 중", "일부 파일을 건너뛰고 완료", "건너뛴 파일: %d"},
        {"Thử lại tệp bị lỗi", "Dọn tệp bị lỗi", "Hoàn tất, đã bỏ qua một số tệp", "Tệp đã bỏ qua: %d"},
        {"Nouvel essai des fichiers", "Suppression des fichiers abîmés", "Terminé avec des fichiers ignorés", "Fichiers ignorés : %d"},
        {"Başarısız dosyalar deneniyor", "Bozuk dosyalar temizleniyor", "Bazı dosyalar atlanarak bitti", "Atlanan dosya: %d"},
        {"Reintentando archivos", "Limpiando archivos dañados", "Finalizado con archivos omitidos", "Archivos omitidos: %d"},
        {"Tentando arquivos novamente", "Limpando arquivos danificados", "Concluído com arquivos ignorados", "Arquivos ignorados: %d"},
        {"Mencoba ulang berkas gagal", "Membersihkan berkas rusak", "Selesai dengan berkas dilewati", "Berkas dilewati: %d"},
        {"Nuovo tentativo dei file", "Pulizia dei file danneggiati", "Completato con file saltati", "File saltati: %d"},
        {"Повтор пошкоджених файлів", "Видалення пошкоджених файлів", "Завершено з пропусками", "Пропущено файлів: %d"},
        {"Försöker filer igen", "Rensar skadade filer", "Klart med överhoppade filer", "Överhoppade filer: %d"},
        {"Prøver filer igen", "Fjerner beskadigede filer", "Færdig med oversprungne filer", "Oversprungne filer: %d"},
        {"Ponawianie plików", "Usuwanie uszkodzonych plików", "Zakończono z pominięciami", "Pominięte pliki: %d"},
    };
    for (unsigned i = 0; i < 4; ++i)
        if (!strcmp(key, texts[0][i])) return texts[lang][i];
    return NULL;
}

static const char *pt_recovery_stop_fallback(const char *key, int lang)
{
    static const char *const texts[DIVOOM_LVGL_MENU_LANG_END][3] = {
        {"Recovery stopped", "Operation timed out", "Retry limit reached"},
        {"自动恢复已停止", "操作超时", "已达到重试次数上限"},
        {"自動恢復已停止", "操作逾時", "已達重試次數上限"},
        {"復旧を停止しました", "処理がタイムアウトしました", "再試行回数の上限に達しました"},
        {"Wiederherstellung angehalten", "Zeitüberschreitung", "Maximale Versuche erreicht"},
        {"หยุดการกู้คืนแล้ว", "การดำเนินการหมดเวลา", "ครบจำนวนครั้งที่ลองใหม่แล้ว"},
        {"Восстановление остановлено", "Время операции истекло", "Лимит повторов исчерпан"},
        {"복구 중지됨", "작업 시간 초과", "재시도 한도에 도달했습니다"},
        {"Đã dừng khôi phục", "Thao tác hết thời gian", "Đã đạt giới hạn thử lại"},
        {"Récupération arrêtée", "Délai dépassé", "Limite de tentatives atteinte"},
        {"Kurtarma durduruldu", "İşlem zaman aşımına uğradı", "Deneme sınırına ulaşıldı"},
        {"Recuperación detenida", "Tiempo de espera agotado", "Límite de intentos alcanzado"},
        {"Recuperação interrompida", "Tempo de operação esgotado", "Limite de tentativas atingido"},
        {"Pemulihan dihentikan", "Waktu operasi habis", "Batas percobaan tercapai"},
        {"Ripristino interrotto", "Tempo operazione scaduto", "Limite di tentativi raggiunto"},
        {"Відновлення зупинено", "Час операції вичерпано", "Досягнуто межі повторів"},
        {"Återställningen stoppad", "Åtgärden tog för lång tid", "Gränsen för omförsök nådd"},
        {"Gendannelse stoppet", "Handlingen fik timeout", "Grænsen for forsøg nået"},
        {"Odzyskiwanie zatrzymane", "Przekroczono czas operacji", "Osiągnięto limit prób"},
    };
    for (unsigned i = 0; i < 3; ++i)
        if (!strcmp(key, texts[0][i])) return texts[lang][i];
    return NULL;
}

static const char *pt_format_confirm_fallback(const char *key, int lang)
{
    unsigned i;
    const char *stopped = pt_recovery_stop_fallback(key, lang);
    if (stopped) return stopped;
    for (i = 0; i < sizeof(s_format_confirm_keys) / sizeof(s_format_confirm_keys[0]); ++i)
        if (!strcmp(key, s_format_confirm_keys[i])) return s_format_confirm_texts[lang][i];
    const char *restore = pt_restore_fallback(key, lang);
    return restore ? restore : pt_integrity_fallback(key, lang);
}

const char *divoom_product_test_system_check_text(int lang)
{
    if (lang < 0 || lang >= DIVOOM_LVGL_MENU_LANG_END) lang = DIVOOM_LVGL_MENU_LANG_ENGLISH;
    return pt_format_confirm_fallback("Check System File", lang);
}

/* Replace legacy path tokens only in these private copies. This also works
 * with language resources already installed on devices. */
static const char *const s_format_partitions[DIVOOM_LVGL_MENU_LANG_END][2] = {
    {"User Partition", "System Partition"},
    {"用户分区", "系统分区"}, {"用戶分區", "系統分區"},
    {"ユーザーパーティション", "システムパーティション"},
    {"Benutzerpartition", "Systempartition"},
    {"พาร์ทิชันผู้ใช้", "พาร์ทิชันระบบ"},
    {"пользовательский раздел", "системный раздел"},
    {"사용자 파티션", "시스템 파티션"},
    {"phân vùng người dùng", "phân vùng hệ thống"},
    {"partition utilisateur", "partition système"},
    {"kullanıcı bölümü", "sistem bölümü"},
    {"partición de usuario", "partición del sistema"},
    {"partição de usuário", "partição do sistema"},
    {"partisi pengguna", "partisi sistem"},
    {"partizione utente", "partizione di sistema"},
    {"розділ користувача", "системний розділ"},
    {"användarpartition", "systempartition"},
    {"brugerpartition", "systempartition"},
    {"partycję użytkownika", "partycję systemową"},
};
#define PT_FORMAT_TEXT_COUNT (sizeof(s_format_keys) / sizeof(s_format_keys[0]))
typedef struct {
    lv_font_t font;
    lv_font_fmt_txt_dsc_t dsc;
    lv_font_fmt_txt_cmap_t cmap;
    lv_font_fmt_txt_glyph_dsc_t *glyphs;
    unsigned char *bitmap;
} pt_format_font;
typedef struct {
    char *texts[PT_FORMAT_TEXT_COUNT];
    uint16_t unicode[PT_FORMAT_GLYPHS];
    int count;
    int pixel_size;
    unsigned references; /* Owned/released on the LVGL thread. */
    pt_format_font fonts[2];
} pt_format_i18n;

const char *divoom_product_test_format_text(void *handle, const char *key,
    const char *english)
{
    pt_format_i18n *ctx = handle;
    unsigned i;
    if (ctx) {
        for (i = 0; i < PT_FORMAT_TEXT_COUNT; ++i)
            if (strcmp(key, s_format_keys[i]) == 0 && ctx->texts[i])
                return ctx->texts[i];
    }
    return english;
}

const lv_font_t *divoom_product_test_format_font(void *handle, int title,
    const lv_font_t *fallback)
{
    pt_format_i18n *ctx = handle;
    (void)title;
    return ctx ? &ctx->fonts[0].font : fallback;
}

void divoom_product_test_format_i18n_destroy(void *handle)
{
    pt_format_i18n *ctx = handle;
    unsigned i;
    if (!ctx) return;
    if (ctx->references > 1) { --ctx->references; return; }
    for (i = 0; i < PT_FORMAT_TEXT_COUNT; ++i)
        divoom_system_free(ctx->texts[i]);
    for (i = 0; i < 2; ++i) {
        divoom_system_free(ctx->fonts[i].glyphs);
        divoom_system_free(ctx->fonts[i].bitmap);
    }
    divoom_system_free(ctx);
}

/* Read the same XML keys as normal menus, without changing their language table. */
static int pt_format_read_texts(pt_format_i18n *ctx, char *xml, int lang)
{
    unsigned i;
    for (i = 0; i < PT_FORMAT_TEXT_COUNT; ++i) {
        char tag[128];
        const char *start, *end;
        char *out;
        size_t n, read, capacity, write = 0;
        snprintf(tag, sizeof(tag), "<string name=\"%s\">", s_format_keys[i]);
        start = strstr(xml, tag);
        if (start) {
            start += strlen(tag);
            end = strstr(start, "</string>");
            if (!end || end == start) start = NULL;
        }
        /* These four labels also replace wording in older installed XML. */
        const char *repair = pt_repair_confirm_fallback(s_format_keys[i], lang);
        if (i < 4 || !start || repair) {
            start = repair ? repair : pt_format_confirm_fallback(s_format_keys[i], lang);
            if (!start) continue;
            end = start + strlen(start);
        }
        n = (size_t)(end - start);
        capacity = n + 1;
        for (read = 0; read < n; ++read)
            if (start[read] == '/')
                capacity += strlen(s_format_partitions[lang][0]) + strlen(s_format_partitions[lang][1]);
        out = divoom_system_malloc(capacity);
        if (!out) return 0;
        for (read = 0; read < n; ++read) {
            int user = read + 9 <= n && memcmp(start + read, "/userdata", 9) == 0;
            int system = read + 4 <= n && memcmp(start + read, "/usr", 4) == 0;
            if (user || system) {
                const char *name = s_format_partitions[lang][!!system];
                size_t size = strlen(name);
                if ((lang == DIVOOM_LVGL_MENU_LANG_SC || lang == DIVOOM_LVGL_MENU_LANG_TC ||
                     lang == DIVOOM_LVGL_MENU_LANG_JP) && write && out[write - 1] == ' ')
                    --write;
                memcpy(out + write, name, size); write += size;
                read += (user ? 9 : 4) - 1;
            } else if (start[read] == '\\' && read + 1 < n && start[read + 1] == 'n') {
                out[write++] = '\n'; ++read;
            } else if (start[read] != '\r') {
                out[write++] = start[read];
            }
        }
        out[write] = 0;
        ctx->texts[i] = out;
    }
    return 1;
}

static int pt_format_add_unicode(pt_format_i18n *ctx, uint32_t cp)
{
    int i;
    if (cp < 32 || cp >= 65535) return 1;
    for (i = 0; i < ctx->count; ++i)
        if (ctx->unicode[i] == cp) return 1;
    if (ctx->count == PT_FORMAT_GLYPHS) return 0;
    ctx->unicode[ctx->count++] = (uint16_t)cp;
    return 1;
}

static int pt_format_collect_unicode(pt_format_i18n *ctx)
{
    unsigned i;
    for (i = 32; i < 127; ++i) pt_format_add_unicode(ctx, i);
    for (i = 0; i <= PT_FORMAT_TEXT_COUNT; ++i) {
        const unsigned char *s = (const unsigned char *)(i == PT_FORMAT_TEXT_COUNT ?
            PT_FACTORY_NEXT_TEXT PT_FACTORY_AGING_TEXT : ctx->texts[i]);
        if (!s) continue;
        while (*s) {
            uint32_t cp = *s++;
            int extra = 0;
            if ((cp & 0xE0) == 0xC0) { cp &= 0x1F; extra = 1; }
            else if ((cp & 0xF0) == 0xE0) { cp &= 0x0F; extra = 2; }
            else if ((cp & 0xF8) == 0xF0) { cp &= 0x07; extra = 3; }
            while (extra--) {
                if ((*s & 0xC0) != 0x80) return 0;
                cp = (cp << 6) | (*s++ & 0x3F);
            }
            if (!pt_format_add_unicode(ctx, cp)) return 0;
        }
    }
    /* Sparse LVGL cmap requires ascending codepoints. */
    for (i = 1; i < (unsigned)ctx->count; ++i) {
        unsigned j = i;
        uint16_t cp = ctx->unicode[i];
        while (j && ctx->unicode[j - 1] > cp) {
            ctx->unicode[j] = ctx->unicode[j - 1]; --j;
        }
        ctx->unicode[j] = cp;
    }
    return 1;
}

static int pt_format_snapshot_font(pt_format_i18n *ctx, FT_Face face, int title)
{
    pt_format_font *out = &ctx->fonts[title];
    size_t pos = 0, capacity = 0;
    uint32 started = divoom_system_get_tickms(), render_ms = 0;
    int i, rendered = 0, ok = 0;
    int pixels = 28;
    if (FT_Set_Pixel_Sizes(face, 0, pixels)) goto finished;
    if (face->size->metrics.height <= 0) goto finished;
    out->glyphs = divoom_system_malloc((ctx->count + 1) * sizeof(*out->glyphs));
    if (!out->glyphs) goto finished;
    memset(out->glyphs, 0, (ctx->count + 1) * sizeof(*out->glyphs));
    for (i = 0; i < ctx->count; ++i) {
        FT_GlyphSlot slot;
        lv_font_fmt_txt_glyph_dsc_t *g = &out->glyphs[i + 1];
        size_t glyph_bytes;
        unsigned y;
        uint32 render_start = divoom_system_get_tickms();
        /* Render each required character once, before FreeType reuses its slot.
         * Missing characters must fail rather than accepting .notdef. */
        int valid = FT_Get_Char_Index(face, ctx->unicode[i]) != 0 &&
            FT_Load_Char(face, ctx->unicode[i], FT_LOAD_RENDER) == 0;
        render_ms += (uint32)(divoom_system_get_tickms() - render_start);
        if (!valid) goto finished;
        ++rendered;
        slot = face->glyph;
        if (slot->bitmap.width && slot->bitmap.rows &&
            (!slot->bitmap.buffer || slot->bitmap.pitch == 0 ||
             (slot->bitmap.pitch > 0 && (unsigned)slot->bitmap.pitch < slot->bitmap.width) ||
             (slot->bitmap.pitch < 0 && -(int64_t)slot->bitmap.pitch < slot->bitmap.width)))
            goto finished;
        if (slot->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY &&
            slot->bitmap.width && slot->bitmap.rows) goto finished;
        if (slot->bitmap.width > 255 || slot->bitmap.rows > 255 ||
            slot->bitmap_left < -128 || slot->bitmap_left > 127 ||
            slot->bitmap_top - (int)slot->bitmap.rows < -128 ||
            slot->bitmap_top - (int)slot->bitmap.rows > 127 ||
            slot->advance.x < 0 || (slot->advance.x + 2) / 4 >= 4096) goto finished;
        glyph_bytes = (size_t)slot->bitmap.width * slot->bitmap.rows;
        if (pos + glyph_bytes >= 1024 * 1024) goto finished;
        if (pos + glyph_bytes > capacity) {
            size_t grown_size = capacity ? capacity : 4096;
            unsigned char *grown;
            while (grown_size < pos + glyph_bytes) grown_size *= 2;
            grown = divoom_system_malloc(grown_size);
            if (!grown) goto finished;
            if (pos) memcpy(grown, out->bitmap, pos);
            divoom_system_free(out->bitmap);
            out->bitmap = grown;
            capacity = grown_size;
        }
        g->bitmap_index = pos;
        g->adv_w = (slot->advance.x + 2) / 4;
        g->box_w = slot->bitmap.width; g->box_h = slot->bitmap.rows;
        g->ofs_x = slot->bitmap_left;
        g->ofs_y = slot->bitmap_top - (int)slot->bitmap.rows;
        for (y = 0; slot->bitmap.width && y < slot->bitmap.rows; ++y) {
            int pitch = slot->bitmap.pitch;
            const unsigned char *row = slot->bitmap.buffer +
                (pitch >= 0 ? (int)y * pitch : ((int)slot->bitmap.rows - 1 - (int)y) * -pitch);
            memcpy(out->bitmap + pos, row, slot->bitmap.width);
            pos += slot->bitmap.width;
        }
    }
    if (!pos) goto finished;
    /* Keep the same compact, contiguous RAM snapshot after temporary growth. */
    if (pos != capacity) {
        unsigned char *packed = divoom_system_malloc(pos);
        if (!packed) goto finished;
        memcpy(packed, out->bitmap, pos);
        divoom_system_free(out->bitmap);
        out->bitmap = packed;
    }
    out->cmap.range_start = 0; out->cmap.range_length = 65535;
    out->cmap.glyph_id_start = 1; out->cmap.unicode_list = ctx->unicode;
    out->cmap.list_length = ctx->count; out->cmap.type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY;
    out->dsc.glyph_bitmap = out->bitmap; out->dsc.glyph_dsc = out->glyphs;
    out->dsc.cmaps = &out->cmap; out->dsc.cmap_num = 1; out->dsc.bpp = 8;
    out->font.get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt;
    out->font.get_glyph_bitmap = lv_font_get_bitmap_fmt_txt;
    out->font.line_height = (face->size->metrics.height + 32) / 64;
    out->font.base_line = -(face->size->metrics.descender / 64);
    out->font.dsc = &out->dsc;
/* 仿真使用的 LVGL 9 开发版没有 static_bitmap，真机保留静态字模标记。 */
#if LVGL_VERSION_MAJOR >= 9 && !DIVOOM_LVGL9_IS_SIM
    out->font.static_bitmap = 1;
#endif
    ok = 1;
finished:
    divoom_print("[PT_FONT] stage=glyphs px=%d glyphs=%d/%d bytes=%u ok=%d render_ms=%u elapsed_ms=%u\n",
        pixels, rendered, ctx->count, (unsigned)pos, ok, (unsigned)render_ms,
        (unsigned)(divoom_system_get_tickms() - started));
    return ok;
}

/* Add NEXT's characters to the selected language before the single render. */
static int pt_format_collect_next_unicode(pt_format_i18n *ctx)
{
    char *first = ctx->texts[0];
    int ok;
#define PT_NEXT_GLYPHS(name, text) text
    ctx->texts[0] = (char *)PT_NEXT_TEXTS(PT_NEXT_GLYPHS);
#undef PT_NEXT_GLYPHS
    ok = pt_format_collect_unicode(ctx);
    ctx->texts[0] = first;
    return ok;
}

static pt_format_i18n *pt_next_snapshot_font(FT_Face face)
{
    pt_format_i18n *ctx = divoom_system_malloc(sizeof(*ctx));
    int ok;
    if (!ctx) return NULL;
    memset(ctx, 0, sizeof(*ctx));
    ctx->pixel_size = 28;
    ctx->references = 1;
    ok = pt_format_collect_next_unicode(ctx);
    if (ok) ok = pt_format_snapshot_font(ctx, face, 0);
    if (!ok) { divoom_product_test_format_i18n_destroy(ctx); ctx = NULL; }
    return ctx;
}

static void *pt_factory_fonts_create(void **next_font)
{
    char path[128], *config, *xml;
    divoom_cJSON *root, *setting, *language;
    pt_format_i18n *ctx = NULL;
    FT_Library library = NULL;
    FT_Face face = NULL;
    int lang = DIVOOM_LVGL_MENU_LANG_ENGLISH, ok = 0, texts_ok = 0, exists;
    FT_Error ft_rc;
    uint32 total_started = divoom_system_get_tickms(), started = total_started;
    if (next_font) *next_font = NULL;
    /* The factory criterion is usability of the required 28px glyphs.
     * Do not scan the whole TTF or read a CRC manifest during preparation. */
    divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_FONT_DATA,
        DIVOOM_MANAGE_IMAGE_ID_BASE_TIME_DIAL_FONT + 28, path);
    exists = divoom_port_file_exists(path);
    divoom_print("[PT_FONT] stage=locate exists=%d check_crc=0 elapsed_ms=%u\n",
        exists, (unsigned)(divoom_system_get_tickms() - started));
    if (!exists) goto done;
    started = divoom_system_get_tickms();
    ft_rc = FT_Init_FreeType(&library);
    if (!ft_rc) ft_rc = FT_New_Face(library, divoom_port_path_resolve(path), 0, &face);
    if (!ft_rc) ft_rc = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    divoom_print("[PT_FONT] stage=open_face rc=%d elapsed_ms=%u\n",
        (int)ft_rc, (unsigned)(divoom_system_get_tickms() - started));
    if (ft_rc) goto done;
    started = divoom_system_get_tickms();
    config = divoom_app_com_read_file(DIVOOM_SYS_CFG_SAVE_PATH, NULL);
    root = config ? divoom_cJSON_Parse(config) : NULL;
    divoom_system_free(config);
    setting = divoom_cJSON_GetObjectItem(root, "sys_setting");
    language = divoom_cJSON_GetObjectItem(setting, "lang");
    if (language && language->type == divoom_cJSON_Number &&
        language->valueint >= 0 && language->valueint < DIVOOM_LVGL_MENU_LANG_END)
        lang = language->valueint;
    divoom_cJSON_Delete(root);
    snprintf(path, sizeof(path), "/usr/share/divoom_app/lang/%s/strings.xml", s_format_locales[lang]);
    xml = divoom_app_com_read_file(path, NULL);
    divoom_print("[PT_FONT] stage=text_read lang=%d ok=%d elapsed_ms=%u\n",
        lang, xml != NULL, (unsigned)(divoom_system_get_tickms() - started));
    started = divoom_system_get_tickms();
    ctx = divoom_system_malloc(sizeof(*ctx));
    if (ctx) {
        memset(ctx, 0, sizeof(*ctx));
        ctx->pixel_size = 28;
        ctx->references = 1;
        texts_ok = xml && pt_format_read_texts(ctx, xml, lang);
        if (!texts_ok) {
            unsigned i;
            for (i = 0; i < PT_FORMAT_TEXT_COUNT; ++i) {
                divoom_system_free(ctx->texts[i]);
                ctx->texts[i] = NULL;
            }
        }
        ok = pt_format_collect_unicode(ctx);
        if (ok && next_font) ok = pt_format_collect_next_unicode(ctx);
    }
    divoom_system_free(xml);
    divoom_print("[PT_FONT] stage=text_parse glyphs=%d text_ok=%d ok=%d elapsed_ms=%u\n",
        ctx ? ctx->count : 0, texts_ok, ok, (unsigned)(divoom_system_get_tickms() - started));
    if (ok) ok = pt_format_snapshot_font(ctx, face, 0);
done:
    started = divoom_system_get_tickms();
    if (face) FT_Done_Face(face);
    if (library) FT_Done_FreeType(library);
    if (!ok) { divoom_product_test_format_i18n_destroy(ctx); ctx = NULL; }
    if (ctx && next_font) {
        *next_font = ctx;
        if (texts_ok) ++ctx->references;
    }
    if (!texts_ok) {
        if (!next_font) divoom_product_test_format_i18n_destroy(ctx);
        ctx = NULL;
    }
    divoom_print("[PT_FONT] stage=cleanup elapsed_ms=%u\n",
        (unsigned)(divoom_system_get_tickms() - started));
    divoom_print("[PT_FONT] stage=total format=%d next=%d check_crc=0 pixels=28 snapshots=%d elapsed_ms=%u\n",
        ctx != NULL, next_font && *next_font != NULL, ok ? 1 : 0,
        (unsigned)(divoom_system_get_tickms() - total_started));
    return ctx;
}

void *divoom_product_test_factory_fonts_create(void **next_font)
{
    return pt_factory_fonts_create(next_font);
}

void *divoom_product_test_format_menu_fonts_create(void)
{
    return pt_factory_fonts_create(NULL);
}

void *divoom_product_test_format_i18n_create(void)
{
    return divoom_product_test_factory_fonts_create(NULL);
}

const lv_font_t *divoom_product_test_next_font(void *handle)
{
    static lv_font_t default_font;
    if (handle) return divoom_product_test_format_font(handle, 0, LV_FONT_DEFAULT);
    /* Preserve the system default's ASCII glyphs; supply local Chinese only. */
    default_font = *LV_FONT_DEFAULT;
    default_font.fallback = &s_pt_default_cjk_font;
    if (default_font.line_height < s_pt_default_cjk_font.line_height)
        default_font.line_height = s_pt_default_cjk_font.line_height;
    default_font.base_line = s_pt_default_cjk_font.base_line;
    return &default_font;
}

void *divoom_product_test_next_font_create(void)
{
    char path[128];
    pt_format_i18n *ctx = NULL;
    FT_Library library = NULL;
    FT_Face face = NULL;
    /* Publish only if the required 28px glyphs can render. No whole-file CRC,
     * download, XML or saved-language dependency in this standalone entry. */
    divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_FONT_DATA,
        DIVOOM_MANAGE_IMAGE_ID_BASE_TIME_DIAL_FONT + 28, path);
    if (!divoom_port_file_exists(path)) return NULL;
    if (FT_Init_FreeType(&library)) return NULL;
    if (FT_New_Face(library, divoom_port_path_resolve(path), 0, &face) ||
        FT_Select_Charmap(face, FT_ENCODING_UNICODE)) goto done;
    ctx = pt_next_snapshot_font(face);
done:
    if (face) FT_Done_Face(face);
    FT_Done_FreeType(library);
    return ctx;
}
