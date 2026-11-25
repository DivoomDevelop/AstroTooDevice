#include "divoom.h"
#include "divoom_menu_control.h"

#define DOWNLOAD_TEST 

typedef struct _divoom_nba_pic_id_item
{
    uint16 pic_id;
    char  pic_flag[48];
}divoom_nba_pic_id_item, *Pdivoom_nba_pic_id_item;

static const divoom_nba_pic_id_item divoom_nba_pic_table[] = //赛程
{
	{DIVOOM_DISP_NBA_BOS, "BOS"},
	{DIVOOM_DISP_NBA_BKN, "BKN"},
	{DIVOOM_DISP_NBA_NYK, "NYK"},
	{DIVOOM_DISP_NBA_PHI, "PHI"},
	{DIVOOM_DISP_NBA_TOR, "TOR"},
	{DIVOOM_DISP_NBA_CHI, "CHI"},
	{DIVOOM_DISP_NBA_CLE, "CLE"},
	{DIVOOM_DISP_NBA_DET, "DET"},
	{DIVOOM_DISP_NBA_IND, "IND"},
	{DIVOOM_DISP_NBA_MIL, "MIL"},
	{DIVOOM_DISP_NBA_ATL, "ATL"},
	{DIVOOM_DISP_NBA_CHA, "CHA"},
	{DIVOOM_DISP_NBA_MIA, "MIA"},
	{DIVOOM_DISP_NBA_ORL, "ORL"},
	{DIVOOM_DISP_NBA_WAS, "WAS"},
	{DIVOOM_DISP_NBA_DAL, "DAL"},
	{DIVOOM_DISP_NBA_HOU, "HOU"},
	{DIVOOM_DISP_NBA_MEM, "MEM"},
	{DIVOOM_DISP_NBA_NOP, "NOP"},
	{DIVOOM_DISP_NBA_SAS, "SAS"},
	{DIVOOM_DISP_NBA_DEN, "DEN"},
	{DIVOOM_DISP_NBA_MIN, "MIN"},
	{DIVOOM_DISP_NBA_OKC, "OKC"},
	{DIVOOM_DISP_NBA_POR, "POR"},
	{DIVOOM_DISP_NBA_UTA, "UTA"},
	{DIVOOM_DISP_NBA_GSW, "GSW"},
	{DIVOOM_DISP_NBA_LAC, "LAC"},
	{DIVOOM_DISP_NBA_LAL, "LAL"},
	{DIVOOM_DISP_NBA_PHX, "PHX"},
	{DIVOOM_DISP_NBA_SAC, "SAC"},
};

static const divoom_nba_pic_id_item divoom_nba2_pic_table[] = 
{
	{DIVOOM_DISP_NBA2_BOS, "BOS"},
	{DIVOOM_DISP_NBA2_BKN, "BKN"},
	{DIVOOM_DISP_NBA2_NYK, "NYK"},
	{DIVOOM_DISP_NBA2_PHI, "PHI"},
	{DIVOOM_DISP_NBA2_TOR, "TOR"},
	{DIVOOM_DISP_NBA2_CHI, "CHI"},
	{DIVOOM_DISP_NBA2_CLE, "CLE"},
	{DIVOOM_DISP_NBA2_DET, "DET"},
	{DIVOOM_DISP_NBA2_IND, "IND"},
	{DIVOOM_DISP_NBA2_MIL, "MIL"},
	{DIVOOM_DISP_NBA2_ATL, "ATL"},
	{DIVOOM_DISP_NBA2_CHA, "CHA"},
	{DIVOOM_DISP_NBA2_MIA, "MIA"},
	{DIVOOM_DISP_NBA2_ORL, "ORL"},
	{DIVOOM_DISP_NBA2_WAS, "WAS"},
	{DIVOOM_DISP_NBA2_DAL, "DAL"},
	{DIVOOM_DISP_NBA2_HOU, "HOU"},
	{DIVOOM_DISP_NBA2_MEM, "MEM"},
	{DIVOOM_DISP_NBA2_NOP, "NOP"},
	{DIVOOM_DISP_NBA2_SAS, "SAS"},
	{DIVOOM_DISP_NBA2_DEN, "DEN"},
	{DIVOOM_DISP_NBA2_MIN, "MIN"},
	{DIVOOM_DISP_NBA2_OKC, "OKC"},
	{DIVOOM_DISP_NBA2_POR, "POR"},
	{DIVOOM_DISP_NBA2_UTA, "UTA"},
	{DIVOOM_DISP_NBA2_GSW, "GSW"},
	{DIVOOM_DISP_NBA2_LAC, "LAC"},
	{DIVOOM_DISP_NBA2_LAL, "LAL"},
	{DIVOOM_DISP_NBA2_PHX, "PHX"},
	{DIVOOM_DISP_NBA2_SAC, "SAC"},
};

static const divoom_nba_pic_id_item divoom_nba3_pic_table[] = //实况
{
	{DIVOOM_DISP_NBA3_BOS, "BOS"},
	{DIVOOM_DISP_NBA3_BKN, "BKN"},
	{DIVOOM_DISP_NBA3_NYK, "NYK"},
	{DIVOOM_DISP_NBA3_PHI, "PHI"},
	{DIVOOM_DISP_NBA3_TOR, "TOR"},
	{DIVOOM_DISP_NBA3_CHI, "CHI"},
	{DIVOOM_DISP_NBA3_CLE, "CLE"},
	{DIVOOM_DISP_NBA3_DET, "DET"},
	{DIVOOM_DISP_NBA3_IND, "IND"},
	{DIVOOM_DISP_NBA3_MIL, "MIL"},
	{DIVOOM_DISP_NBA3_ATL, "ATL"},
	{DIVOOM_DISP_NBA3_CHA, "CHA"},
	{DIVOOM_DISP_NBA3_MIA, "MIA"},
	{DIVOOM_DISP_NBA3_ORL, "ORL"},
	{DIVOOM_DISP_NBA3_WAS, "WAS"},
	{DIVOOM_DISP_NBA3_DAL, "DAL"},
	{DIVOOM_DISP_NBA3_HOU, "HOU"},
	{DIVOOM_DISP_NBA3_MEM, "MEM"},
	{DIVOOM_DISP_NBA3_NOP, "NOP"},
	{DIVOOM_DISP_NBA3_SAS, "SAS"},
	{DIVOOM_DISP_NBA3_DEN, "DEN"},
	{DIVOOM_DISP_NBA3_MIN, "MIN"},
	{DIVOOM_DISP_NBA3_OKC, "OKC"},
	{DIVOOM_DISP_NBA3_POR, "POR"},
	{DIVOOM_DISP_NBA3_UTA, "UTA"},
	{DIVOOM_DISP_NBA3_GSW, "GSW"},
	{DIVOOM_DISP_NBA3_LAC, "LAC"},
	{DIVOOM_DISP_NBA3_LAL, "LAL"},
	{DIVOOM_DISP_NBA3_PHX, "PHX"},
	{DIVOOM_DISP_NBA3_SAC, "SAC"},
};


static const divoom_nba_pic_id_item divoom_nfl_pic_table[] = 
{
	{DIVOOM_DISP_NFL_NEP, "NEP"},
	{DIVOOM_DISP_NFL_NYJ, "NYJ"},
	{DIVOOM_DISP_NFL_BUF, "BUF"},
	{DIVOOM_DISP_NFL_MIA, "MIA"},
	{DIVOOM_DISP_NFL_TEN, "TEN"},
	{DIVOOM_DISP_NFL_IND, "IND"},
	{DIVOOM_DISP_NFL_JAX, "JAX"},
	{DIVOOM_DISP_NFL_HOU, "HOU"},
	{DIVOOM_DISP_NFL_BAL, "BAL"},
	{DIVOOM_DISP_NFL_PIT, "PIT"},
	{DIVOOM_DISP_NFL_CLE, "CLE"},
	{DIVOOM_DISP_NFL_CIN, "CIN"},
	{DIVOOM_DISP_NFL_KC, "KC"},
	{DIVOOM_DISP_NFL_LV, "LV"},
	{DIVOOM_DISP_NFL_LAC, "LAC"},
	{DIVOOM_DISP_NFL_DEN, "DEN"},
	{DIVOOM_DISP_NFL_DAL, "DAL"},
	{DIVOOM_DISP_NFL_NYG, "NYG"},
	{DIVOOM_DISP_NFL_WAS, "WAS"},
	{DIVOOM_DISP_NFL_PHI, "PHI"},
	{DIVOOM_DISP_NFL_NO, "NO"},
	{DIVOOM_DISP_NFL_ATL, "ATL"},
	{DIVOOM_DISP_NFL_CAR, "CAR"},
	{DIVOOM_DISP_NFL_TB, "TB"},
	{DIVOOM_DISP_NFL_CHI, "CHI"},
	{DIVOOM_DISP_NFL_MIN, "MIN"},
	{DIVOOM_DISP_NFL_DET, "DET"},
	{DIVOOM_DISP_NFL_GB, "GB"},
	{DIVOOM_DISP_NFL_SF, "SF"},
	{DIVOOM_DISP_NFL_LAR, "LAR"},
	{DIVOOM_DISP_NFL_SEA, "SEA"},
	{DIVOOM_DISP_NFL_ARI, "ARI"},
};

static const divoom_nba_pic_id_item divoom_euro_cup_pic_table[] = 
{
	{DIVOOM_DISP_EURO_CUP_GER, "GER"},
	{DIVOOM_DISP_EURO_CUP_CHE, "CHE"},
	{DIVOOM_DISP_EURO_CUP_HUN, "HUN"},
	{DIVOOM_DISP_EURO_CUP_SCO, "SCO"},
	{DIVOOM_DISP_EURO_CUP_ESP, "ESP"},
	{DIVOOM_DISP_EURO_CUP_HRV, "HRV"},
	{DIVOOM_DISP_EURO_CUP_HRV, "CRO"},
	{DIVOOM_DISP_EURO_CUP_ITA, "ITA"},
	{DIVOOM_DISP_EURO_CUP_ALB, "ALB"},
	{DIVOOM_DISP_EURO_CUP_ENG, "ENG"},
	{DIVOOM_DISP_EURO_CUP_DNK, "DNK"},
	{DIVOOM_DISP_EURO_CUP_SVN, "SVN"},
	{DIVOOM_DISP_EURO_CUP_SRB, "SRB"},
	{DIVOOM_DISP_EURO_CUP_FRA, "FRA"},
	{DIVOOM_DISP_EURO_CUP_NLD, "NLD"},
	{DIVOOM_DISP_EURO_CUP_AUT, "AUT"},
	{DIVOOM_DISP_EURO_CUP_POL, "POL"},
	{DIVOOM_DISP_EURO_CUP_ROU, "ROU"},
	{DIVOOM_DISP_EURO_CUP_BEL, "BEL"},
	{DIVOOM_DISP_EURO_CUP_SVK, "SVK"},
	{DIVOOM_DISP_EURO_CUP_UKR, "UKR"},
	{DIVOOM_DISP_EURO_CUP_PRT, "PRT"},
	{DIVOOM_DISP_EURO_CUP_TUR, "TUR"},
	{DIVOOM_DISP_EURO_CUP_GEO, "GEO"},
	{DIVOOM_DISP_EURO_CUP_CZE, "CZE"},
};

static const divoom_nba_pic_id_item divoom_premier_league_pic_table[] = //英超
{
	{DIVOOM_DISP_PREMIER_LEAGUE_LIVERPOOL, "Liverpool"},
	{DIVOOM_DISP_PREMIER_LEAGUE_ARSENALl, "Arsenal"},
	{DIVOOM_DISP_PREMIER_LEAGUE_NOTTINGHAM, "Nottingham Forest"},
	{DIVOOM_DISP_PREMIER_LEAGUE_CHELSEA, "Chelsea"},
	{DIVOOM_DISP_PREMIER_LEAGUE_MANCITY, "Manchester City"},
	{DIVOOM_DISP_PREMIER_LEAGUE_NEWCASTLE, "Newcastle"},
	{DIVOOM_DISP_PREMIER_LEAGUE_BRIGHTON, "Brighton"},
	{DIVOOM_DISP_PREMIER_LEAGUE_FULHAM, "Fulham"},
	{DIVOOM_DISP_PREMIER_LEAGUE_VILLA, "Aston Villa"},
	{DIVOOM_DISP_PREMIER_LEAGUE_BOURNEMOUTH, "Bournemouth"},
	{DIVOOM_DISP_PREMIER_LEAGUE_BRENTFORD, "Brentford"},
	{DIVOOM_DISP_PREMIER_LEAGUE_CRYSTALPALACE, "Crystal Palace"},
	{DIVOOM_DISP_PREMIER_LEAGUE_MANCHESTERUNITED, "Manchester United"},
	{DIVOOM_DISP_PREMIER_LEAGUE_HOTSPUR, "Tottenham"},
	{DIVOOM_DISP_PREMIER_LEAGUE_EVERTON, "Everton"},
	{DIVOOM_DISP_PREMIER_LEAGUE_WESTHAM, "West Ham"},
	{DIVOOM_DISP_PREMIER_LEAGUE_WOLVES, "Wolves"},
	{DIVOOM_DISP_PREMIER_LEAGUE_IPSWICH, "Ipswich"},
	{DIVOOM_DISP_PREMIER_LEAGUE_LEICESTERCITY, "Leicester"},
	{DIVOOM_DISP_PREMIER_LEAGUE_SOUTHAMPTON, "Southampton"},
	{DIVOOM_DISP_PREMIER_LEAGUE_Sunderland, "Sunderland"},
	{DIVOOM_DISP_PREMIER_LEAGUE_Leeds, "Leeds"},
	{DIVOOM_DISP_PREMIER_LEAGUE_Burnley, "Burnley"},
	{DIVOOM_DISP_PREMIER_LEAGUE_HullCity, "Hull City"},
	{DIVOOM_DISP_PREMIER_LEAGUE_Coventry, "Coventry"},
};

static const divoom_nba_pic_id_item divoom_premier_league_abbrev_table[] =
{
	{DIVOOM_DISP_PREMIER_LEAGUE_LIVERPOOL, "LIV"},
	{DIVOOM_DISP_PREMIER_LEAGUE_ARSENALl, "ARS"},
	{DIVOOM_DISP_PREMIER_LEAGUE_NOTTINGHAM, "NFO"},
	{DIVOOM_DISP_PREMIER_LEAGUE_CHELSEA, "CHE"},
	{DIVOOM_DISP_PREMIER_LEAGUE_MANCITY, "MCI"},
	{DIVOOM_DISP_PREMIER_LEAGUE_NEWCASTLE, "NEW"},
	{DIVOOM_DISP_PREMIER_LEAGUE_BRIGHTON, "BHA"},
	{DIVOOM_DISP_PREMIER_LEAGUE_VILLA, "AVL"},
	{DIVOOM_DISP_PREMIER_LEAGUE_BOURNEMOUTH, "BOU"},
	{DIVOOM_DISP_PREMIER_LEAGUE_FULHAM, "FUL"},
	{DIVOOM_DISP_PREMIER_LEAGUE_CRYSTALPALACE, "CRY"},
	{DIVOOM_DISP_PREMIER_LEAGUE_BRENTFORD, "BRE"},
	{DIVOOM_DISP_PREMIER_LEAGUE_HOTSPUR, "TOT"},
	{DIVOOM_DISP_PREMIER_LEAGUE_EVERTON, "EVE"},
	{DIVOOM_DISP_PREMIER_LEAGUE_WESTHAM, "WHU"},
	{DIVOOM_DISP_PREMIER_LEAGUE_WOLVES, "WOL"},
	{DIVOOM_DISP_PREMIER_LEAGUE_IPSWICH, "IPS"},
	{DIVOOM_DISP_PREMIER_LEAGUE_LEICESTERCITY, "LEI"},
	{DIVOOM_DISP_PREMIER_LEAGUE_SOUTHAMPTON, "SOU"},
	{DIVOOM_DISP_PREMIER_LEAGUE_MANCHESTERUNITED, "MUN"},
	{DIVOOM_DISP_PREMIER_LEAGUE_Sunderland, "SUN"},
	{DIVOOM_DISP_PREMIER_LEAGUE_Leeds, "LEE"},
	{DIVOOM_DISP_PREMIER_LEAGUE_Burnley, "BUR"},
	{DIVOOM_DISP_PREMIER_LEAGUE_HullCity, "HUL"},
	{DIVOOM_DISP_PREMIER_LEAGUE_Coventry, "COV"},
};

static const divoom_nba_pic_id_item divoom_laliga_pic_table[] = //西甲
{
	{DIVOOM_DISP_LALIGA_Barcelona, "Barcelona"},
	{DIVOOM_DISP_LALIGA_RealMadrid, "Real Madrid"},
	{DIVOOM_DISP_LALIGA_AtleticoMadrid, "Atletico Madrid"},
	{DIVOOM_DISP_LALIGA_AthleticClub, "Athletic Club"},
	{DIVOOM_DISP_LALIGA_Villarreal, "Villarreal"},
	{DIVOOM_DISP_LALIGA_RealBetis, "Real Betis"},
	{DIVOOM_DISP_LALIGA_Mallorca, "Mallorca"},
	{DIVOOM_DISP_LALIGA_CeltaVigo, "Celta Vigo"},
	{DIVOOM_DISP_LALIGA_RayoVallecano, "Rayo Vallecano"},
	{DIVOOM_DISP_LALIGA_Sevilla, "Sevilla"},
	{DIVOOM_DISP_LALIGA_Getafe, "Getafe"},
	{DIVOOM_DISP_LALIGA_RealSociedad, "Real Sociedad"},
	{DIVOOM_DISP_LALIGA_Girona, "Girona"},
	{DIVOOM_DISP_LALIGA_Osasuna, "Osasuna"},
	{DIVOOM_DISP_LALIGA_Espanyol, "Espanyol"},
	{DIVOOM_DISP_LALIGA_Valencia, "Valencia"},
	{DIVOOM_DISP_LALIGA_Alaves, "Alaves"},
	{DIVOOM_DISP_LALIGA_Leganes, "Leganes"},
	{DIVOOM_DISP_LALIGA_LasPalmas, "Las Palmas"},
	{DIVOOM_DISP_LALIGA_Valladolid, "Valladolid"},
	{DIVOOM_DISP_LALIGA_Elche, "Elche"},
	{DIVOOM_DISP_LALIGA_Oviedo, "Oviedo"},
	{DIVOOM_DISP_LALIGA_Levante, "Levante"},
	{DIVOOM_DISP_LALIGA_DeportivoLaCoruna, "Deportivo La Coruna"},
	{DIVOOM_DISP_LALIGA_RacingSantander, "Racing Santander"},
	{DIVOOM_DISP_LALIGA_Malaga, "Malaga"},
};

static const divoom_nba_pic_id_item divoom_laliga_abbrev_table[] =
{
	{DIVOOM_DISP_LALIGA_Barcelona, "BAR"},
	{DIVOOM_DISP_LALIGA_RealMadrid, "RMA"},
	{DIVOOM_DISP_LALIGA_AtleticoMadrid, "ATM"},
	{DIVOOM_DISP_LALIGA_AthleticClub, "ATH"},
	{DIVOOM_DISP_LALIGA_Villarreal, "VIL"},
	{DIVOOM_DISP_LALIGA_RealBetis, "BET"},
	{DIVOOM_DISP_LALIGA_Mallorca, "MLL"},
	{DIVOOM_DISP_LALIGA_CeltaVigo, "CEL"},
	{DIVOOM_DISP_LALIGA_RayoVallecano, "RAY"},
	{DIVOOM_DISP_LALIGA_Sevilla, "SEV"},
	{DIVOOM_DISP_LALIGA_Getafe, "GET"},
	{DIVOOM_DISP_LALIGA_RealSociedad, "RSO"},
	{DIVOOM_DISP_LALIGA_Girona, "GIR"},
	{DIVOOM_DISP_LALIGA_Osasuna, "OSA"},
	{DIVOOM_DISP_LALIGA_Espanyol, "ESP"},
	{DIVOOM_DISP_LALIGA_Valencia, "VAL"},
	{DIVOOM_DISP_LALIGA_Alaves, "ALA"},
	{DIVOOM_DISP_LALIGA_Leganes, "LEG"},
	{DIVOOM_DISP_LALIGA_LasPalmas, "LPA"},
	{DIVOOM_DISP_LALIGA_Valladolid, "VLL"},
	{DIVOOM_DISP_LALIGA_Elche, "ELC"},
	{DIVOOM_DISP_LALIGA_Oviedo, "ROV"},
	{DIVOOM_DISP_LALIGA_Levante, "LEV"},
	{DIVOOM_DISP_LALIGA_DeportivoLaCoruna, "DEP"},
	{DIVOOM_DISP_LALIGA_RacingSantander, "RAC"},
	{DIVOOM_DISP_LALIGA_Malaga, "MAL"},
};

static const divoom_nba_pic_id_item divoom_seriea_pic_table[] = //意甲
{
	{DIVOOM_DISP_SERIEA_Inter, "Inter"},
	{DIVOOM_DISP_SERIEA_Napoli, "Napoli"},
	{DIVOOM_DISP_SERIEA_Atalanta, "Atalanta"},
	{DIVOOM_DISP_SERIEA_Bologna, "Bologna"},
	{DIVOOM_DISP_SERIEA_Juventus, "Juventus"},
	{DIVOOM_DISP_SERIEA_Lazio, "Lazio"},
	{DIVOOM_DISP_SERIEA_ASRoma, "AS Roma"},
	{DIVOOM_DISP_SERIEA_Fiorentina, "Fiorentina"},
	{DIVOOM_DISP_SERIEA_ACMilan, "AC Milan"},
	{DIVOOM_DISP_SERIEA_Udinese, "Udinese"},
	{DIVOOM_DISP_SERIEA_Torino, "Torino"},
	{DIVOOM_DISP_SERIEA_Genoa, "Genoa"},
	{DIVOOM_DISP_SERIEA_Como, "Como"},
	{DIVOOM_DISP_SERIEA_Verona, "Verona"},
	{DIVOOM_DISP_SERIEA_Verona, "Hellas Verona"},
	{DIVOOM_DISP_SERIEA_Cagliari, "Cagliari"},
	{DIVOOM_DISP_SERIEA_Lecce, "Lecce"},
	{DIVOOM_DISP_SERIEA_Parma, "Parma"},
	{DIVOOM_DISP_SERIEA_Empoli, "Empoli"},
	{DIVOOM_DISP_SERIEA_Venezia, "Venezia"},
	{DIVOOM_DISP_SERIEA_Monza, "Monza"},
	{DIVOOM_DISP_SERIEA_Cremonese, "Cremonese"},
	{DIVOOM_DISP_SERIEA_Pisa, "Pisa"},
	{DIVOOM_DISP_SERIEA_Sassuolo, "Sassuolo"},
	{DIVOOM_DISP_SERIEA_Frosinone, "Frosinone"},
};

static const divoom_nba_pic_id_item divoom_seriea_abbrev_table[] =
{
	{DIVOOM_DISP_SERIEA_Inter, "INT"},
	{DIVOOM_DISP_SERIEA_Napoli, "NAP"},
	{DIVOOM_DISP_SERIEA_Atalanta, "ATA"},
	{DIVOOM_DISP_SERIEA_Bologna, "BOL"},
	{DIVOOM_DISP_SERIEA_Juventus, "JUV"},
	{DIVOOM_DISP_SERIEA_Lazio, "LAZ"},
	{DIVOOM_DISP_SERIEA_ASRoma, "ROM"},
	{DIVOOM_DISP_SERIEA_Fiorentina, "FIO"},
	{DIVOOM_DISP_SERIEA_ACMilan, "MIL"},
	{DIVOOM_DISP_SERIEA_Udinese, "UDI"},
	{DIVOOM_DISP_SERIEA_Torino, "TOR"},
	{DIVOOM_DISP_SERIEA_Genoa, "GEN"},
	{DIVOOM_DISP_SERIEA_Como, "CMO"},
	{DIVOOM_DISP_SERIEA_Verona, "VER"},
	{DIVOOM_DISP_SERIEA_Cagliari, "CAG"},
	{DIVOOM_DISP_SERIEA_Lecce, "LEC"},
	{DIVOOM_DISP_SERIEA_Parma, "PAR"},
	{DIVOOM_DISP_SERIEA_Empoli, "EMP"},
	{DIVOOM_DISP_SERIEA_Venezia, "VEN"},
	{DIVOOM_DISP_SERIEA_Monza, "MON"},
	{DIVOOM_DISP_SERIEA_Cremonese, "CRE"},
	{DIVOOM_DISP_SERIEA_Pisa, "PIS"},
	{DIVOOM_DISP_SERIEA_Sassuolo, "SAS"},
	{DIVOOM_DISP_SERIEA_Frosinone, "FRO"},
};

static const divoom_nba_pic_id_item divoom_bundesliga_pic_table[] = //德甲
{
	{DIVOOM_DISP_BUNDESLIGA_Bayern, "Bayern München"},
	{DIVOOM_DISP_BUNDESLIGA_Leverkusen, "Bayer Leverkusen"},
	{DIVOOM_DISP_BUNDESLIGA_Mainz, "FSV Mainz 05"},
	{DIVOOM_DISP_BUNDESLIGA_Frankfurt, "Eintracht Frankfurt"},
	{DIVOOM_DISP_BUNDESLIGA_Freiburg, "SC Freiburg"},
	{DIVOOM_DISP_BUNDESLIGA_Leipzig, "RB Leipzig"},
	{DIVOOM_DISP_BUNDESLIGA_Gladbach, "Borussia Mönchengladbach"},
	{DIVOOM_DISP_BUNDESLIGA_Wolfsburg, "VfL Wolfsburg"},
	{DIVOOM_DISP_BUNDESLIGA_Augsburg, "FC Augsburg"},
	{DIVOOM_DISP_BUNDESLIGA_Stuttgart, "VfB Stuttgart"}, 
	{DIVOOM_DISP_BUNDESLIGA_Dortmund, "Borussia Dortmund"},
	{DIVOOM_DISP_BUNDESLIGA_Bremen, "Werder Bremen"},
	{DIVOOM_DISP_BUNDESLIGA_UnionBerlin, "Union Berlin"},
	{DIVOOM_DISP_BUNDESLIGA_Hoffenheim, "1899 Hoffenheim"},
	{DIVOOM_DISP_BUNDESLIGA_StPauli, "FC St. Pauli"},
	{DIVOOM_DISP_BUNDESLIGA_Bochum, "VfL Bochum"},
	{DIVOOM_DISP_BUNDESLIGA_Heidenheim, "1. FC Heidenheim"},
	{DIVOOM_DISP_BUNDESLIGA_HolsteinKiel, "Holstein Kiel"},
	{DIVOOM_DISP_BUNDESLIGA_1FCKln, "1.FC Köln"},
	{DIVOOM_DISP_BUNDESLIGA_1FCKln, "1. FC Köln"},
	{DIVOOM_DISP_BUNDESLIGA_HamburgerSV, "Hamburger SV"},
	{DIVOOM_DISP_BUNDESLIGA_Elversberg, "SV Elversberg"},
	{DIVOOM_DISP_BUNDESLIGA_Schalke, "FC Schalke 04"},
	{DIVOOM_DISP_BUNDESLIGA_Paderborn, "SC Paderborn 07"},
};

static const divoom_nba_pic_id_item divoom_bundesliga_abbrev_table[] =
{
	{DIVOOM_DISP_BUNDESLIGA_Bayern, "FCB"},
	{DIVOOM_DISP_BUNDESLIGA_Leverkusen, "LEV"},
	{DIVOOM_DISP_BUNDESLIGA_Mainz, "MAI"},
	{DIVOOM_DISP_BUNDESLIGA_Frankfurt, "SGE"},
	{DIVOOM_DISP_BUNDESLIGA_Freiburg, "SCF"},
	{DIVOOM_DISP_BUNDESLIGA_Leipzig, "RBL"},
	{DIVOOM_DISP_BUNDESLIGA_Gladbach, "BMG"},
	{DIVOOM_DISP_BUNDESLIGA_Wolfsburg, "WOB"},
	{DIVOOM_DISP_BUNDESLIGA_Augsburg, "FCA"},
	{DIVOOM_DISP_BUNDESLIGA_Stuttgart, "VFB"},
	{DIVOOM_DISP_BUNDESLIGA_Dortmund, "BVB"},
	{DIVOOM_DISP_BUNDESLIGA_Bremen, "SVW"},
	{DIVOOM_DISP_BUNDESLIGA_UnionBerlin, "FCU"},
	{DIVOOM_DISP_BUNDESLIGA_Hoffenheim, "TSG"},
	{DIVOOM_DISP_BUNDESLIGA_StPauli, "STP"},
	{DIVOOM_DISP_BUNDESLIGA_Bochum, "BOC"},
	{DIVOOM_DISP_BUNDESLIGA_Heidenheim, "HDH"},
	{DIVOOM_DISP_BUNDESLIGA_HolsteinKiel, "KIE"},
	{DIVOOM_DISP_BUNDESLIGA_1FCKln, "KOE"},
	{DIVOOM_DISP_BUNDESLIGA_HamburgerSV, "HSV"},
	{DIVOOM_DISP_BUNDESLIGA_Elversberg, "ELV"},
	{DIVOOM_DISP_BUNDESLIGA_Schalke, "S04"},
	{DIVOOM_DISP_BUNDESLIGA_Paderborn, "PAD"},
};

static const divoom_nba_pic_id_item divoom_mlb_pic_table[] = {
    {DIVOOM_DISP_MLB_NYY, "NYY"},
    {DIVOOM_DISP_MLB_CLE, "CLE"},
    {DIVOOM_DISP_MLB_BAL, "BAL"},
    {DIVOOM_DISP_MLB_HOU, "HOU"},
    {DIVOOM_DISP_MLB_KC, "KC"},
    {DIVOOM_DISP_MLB_DET, "DET"},
    {DIVOOM_DISP_MLB_SEA, "SEA"},
    {DIVOOM_DISP_MLB_MIN, "MIN"},
    {DIVOOM_DISP_MLB_BOS, "BOS"},
    {DIVOOM_DISP_MLB_TB, "TB"},
    {DIVOOM_DISP_MLB_TEX, "TEX"},
    {DIVOOM_DISP_MLB_TOR, "TOR"},
    {DIVOOM_DISP_MLB_OAK, "OAK"},
    {DIVOOM_DISP_MLB_LAA, "LAA"},
    {DIVOOM_DISP_MLB_CWS, "CWS"},
    {DIVOOM_DISP_MLB_LAD, "LAD"},
    {DIVOOM_DISP_MLB_PHI, "PHI"},
    {DIVOOM_DISP_MLB_MIL, "MIL"},
    {DIVOOM_DISP_MLB_SD, "SD"},
    {DIVOOM_DISP_MLB_ATL, "ATL"},
    {DIVOOM_DISP_MLB_NYM, "NYM"},
    {DIVOOM_DISP_MLB_ARI, "ARI"},
    {DIVOOM_DISP_MLB_CHC, "CHC"},
    {DIVOOM_DISP_MLB_STL, "STL"},
    {DIVOOM_DISP_MLB_SF, "SF"},
    {DIVOOM_DISP_MLB_CIN, "CIN"},
    {DIVOOM_DISP_MLB_PIT, "PIT"},
    {DIVOOM_DISP_MLB_WSH, "WSH"},
    {DIVOOM_DISP_MLB_MIA, "MIA"},
    {DIVOOM_DISP_MLB_COL, "COL"},    
};

static const divoom_nba_pic_id_item divoom_afl_pic_table[] = {
	{DIVOOM_DISP_AFL_GWS_Giants, "GWS Giants"},
	{DIVOOM_DISP_AFL_Hawthorn, "Hawthorn"},
	{DIVOOM_DISP_AFL_Gold_Coast_Suns, "Gold Coast Suns"},
	{DIVOOM_DISP_AFL_Geelong_Cats, "Geelong Cats"},
	{DIVOOM_DISP_AFL_Adelaide_Crows, "Adelaide Crows"},
	{DIVOOM_DISP_AFL_Collingwood, "Collingwood"},
	{DIVOOM_DISP_AFL_Richmond, "Richmond"},
	{DIVOOM_DISP_AFL_Western_Bulldogs, "Western Bulldogs"},
	{DIVOOM_DISP_AFL_Brisbane_Lions, "Brisbane Lions"},
	{DIVOOM_DISP_AFL_Carlton, "Carlton"},
	{DIVOOM_DISP_AFL_Melbourne, "Melbourne"},
	{DIVOOM_DISP_AFL_Sydney_Swans, "Sydney Swans"},
	{DIVOOM_DISP_AFL_North_Melbourne, "North Melbourne"},
	{DIVOOM_DISP_AFL_Essendon, "Essendon"},
	{DIVOOM_DISP_AFL_St_Kilda, "St Kilda"},
	{DIVOOM_DISP_AFL_Fremantle, "Fremantle"},
	{DIVOOM_DISP_AFL_West_Coast_Eagles, "West Coast Eagles"},
	{DIVOOM_DISP_AFL_Port_Adelaide, "Port Adelaide"},
};

static int divoom_nba_pic_lookup_id(char *team_name, int team_type, int nba_flag);

void divoom_nba_pic_save(Pdivoom_team_img_item pitem, int number, int team_type)
{
    char *json_info;
    divoom_cJSON *root, *list, *item;

	root = divoom_cJSON_CreateObject();
    if(root == NULL)
    {
        return;
    }
    
	divoom_cJSON_AddItemToObject(root, "TeameList", list = divoom_cJSON_CreateArray());
    for(int i = 0; i < number; i++)
    {
		divoom_cJSON_AddItemToArray(list, item = divoom_cJSON_CreateObject());  
		divoom_cJSON_AddNumberToObject(item, "pic_id", pitem[i].pic_id);
        divoom_cJSON_AddStringToObject(item , "name", pitem[i].team_name); 
        divoom_cJSON_AddStringToObject(item , "image_url1", pitem[i].image_url1); 
		if (team_type == 0)
		{
			divoom_cJSON_AddNumberToObject(item, "pic_id2", pitem[i].pic_id2);
			divoom_cJSON_AddNumberToObject(item, "pic_id3", pitem[i].pic_id3);
	        divoom_cJSON_AddStringToObject(item , "image_url2", pitem[i].image_url2); 
			divoom_cJSON_AddStringToObject(item , "image_url3", pitem[i].image_url3); 
		}
    }
    json_info = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);

	if (team_type == 0)
	    divoom_app_com_write_file(DIVOOM_ALL_NBA_PIC_SAVE_PATH, (uint8 *)json_info, strlen(json_info));
	else if (team_type == 1)
		divoom_app_com_write_file(DIVOOM_ALL_EURCUP_PIC_SAVE_PATH, (uint8 *)json_info, strlen(json_info));
	else if (team_type == 2)
		divoom_app_com_write_file(DIVOOM_ALL_NFL_PIC_SAVE_PATH, (uint8 *)json_info, strlen(json_info));
	else if (team_type == 3)
		divoom_app_com_write_file(DIVOOM_ALL_PREMIER_LEAGUE_PIC_SAVE_PATH, (uint8 *)json_info, strlen(json_info));
	else if (team_type == 4)
		divoom_app_com_write_file(DIVOOM_ALL_LALIGA_PIC_SAVE_PATH, (uint8 *)json_info, strlen(json_info));
	else if (team_type == 5)
		divoom_app_com_write_file(DIVOOM_ALL_SERIEA_PIC_SAVE_PATH, (uint8 *)json_info, strlen(json_info));
	else if (team_type == 6)
		divoom_app_com_write_file(DIVOOM_ALL_BUNDESLIGA_PIC_SAVE_PATH, (uint8 *)json_info, strlen(json_info));
	else if (team_type == 7)
		divoom_app_com_write_file(DIVOOM_ALL_MLB_PIC_SAVE_PATH, (uint8 *)json_info, strlen(json_info));
	else if (team_type == 8)
		divoom_app_com_write_file(DIVOOM_ALL_AFL_PIC_SAVE_PATH, (uint8 *)json_info, strlen(json_info));

    divoom_system_free(json_info);
}

/* 根据赛事类型返回主配置或出厂备份路径，写入方继续使用原有主路径。 */
static char *divoom_nba_pic_get_save_path(int team_type, int use_backup)
{
	switch (team_type)
	{
	case 0:
	{
		return use_backup ? DIVOOM_ALL_NBA_PIC_SAVE_BAK_PATH : DIVOOM_ALL_NBA_PIC_SAVE_PATH;
	}
	case 1:
	{
		return use_backup ? DIVOOM_ALL_EURCUP_PIC_SAVE_BAK_PATH : DIVOOM_ALL_EURCUP_PIC_SAVE_PATH;
	}
	case 2:
	{
		return use_backup ? DIVOOM_ALL_NFL_PIC_SAVE_BAK_PATH : DIVOOM_ALL_NFL_PIC_SAVE_PATH;
	}
	case 3:
	{
		return use_backup ? DIVOOM_ALL_PREMIER_LEAGUE_PIC_SAVE_BAK_PATH : DIVOOM_ALL_PREMIER_LEAGUE_PIC_SAVE_PATH;
	}
	case 4:
	{
		return use_backup ? DIVOOM_ALL_LALIGA_PIC_SAVE_BAK_PATH : DIVOOM_ALL_LALIGA_PIC_SAVE_PATH;
	}
	case 5:
	{
		return use_backup ? DIVOOM_ALL_SERIEA_PIC_SAVE_BAK_PATH : DIVOOM_ALL_SERIEA_PIC_SAVE_PATH;
	}
	case 6:
	{
		return use_backup ? DIVOOM_ALL_BUNDESLIGA_PIC_SAVE_BAK_PATH : DIVOOM_ALL_BUNDESLIGA_PIC_SAVE_PATH;
	}
	case 7:
	{
		return use_backup ? DIVOOM_ALL_MLB_PIC_SAVE_BAK_PATH : DIVOOM_ALL_MLB_PIC_SAVE_PATH;
	}
	case 8:
	{
		return use_backup ? DIVOOM_ALL_AFL_PIC_SAVE_BAK_PATH : DIVOOM_ALL_AFL_PIC_SAVE_PATH;
	}
	default:
	{
		return NULL;
	}
	}
}

/* 主配置或备份任一存在，都可用于注册本地赛事图片列表。 */
static int divoom_nba_pic_check_team_file_exist(int team_type)
{
	char *path;

	path = divoom_nba_pic_get_save_path(team_type, 0);
	if (path && divoom_app_com_check_file_exist(path) != 0)
	{
		return 1;
	}
	path = divoom_nba_pic_get_save_path(team_type, 1);
	if (path && divoom_app_com_check_file_exist(path) != 0)
	{
		return 1;
	}
	return 0;
}

Pdivoom_team_img_item divoom_nba_pic_load(int *number, int team_type)
{
    char *file_buf = NULL;
    divoom_cJSON *root, *list, *item;
    Pdivoom_team_img_item pitem;
	char *path;

	path = divoom_nba_pic_get_save_path(team_type, 0);
	if (path)
	{
		file_buf = divoom_app_com_read_file(path, NULL);
	}
	if (file_buf == NULL)
	{
		/* 主配置读取失败时回退到同赛事的出厂备份。 */
		path = divoom_nba_pic_get_save_path(team_type, 1);
		if (path)
		{
			file_buf = divoom_app_com_read_file(path, NULL);
		}
	}

    if(file_buf == NULL)
    {
        return NULL;
    }
    root = divoom_cJSON_Parse(file_buf);
    divoom_system_free(file_buf);
    if(root == NULL)
    {
        return NULL;
    }

    list = divoom_cJSON_GetObjectItem(root, "TeameList");
    *number = divoom_cJSON_GetArraySize(list);
    pitem = divoom_system_malloc(*number * sizeof(divoom_team_img_item));
    for(int i = 0; i < *number; i++)
    {
        item = divoom_cJSON_GetArrayItem(list, i);
        divoom_cJSON_GetObjectString(item, "name", pitem[i].team_name);
        divoom_cJSON_GetObjectString(item, "image_url1", pitem[i].image_url1);
		if (team_type == 0)
		{
			divoom_cJSON_GetObjectString(item, "image_url2", pitem[i].image_url2);
	        divoom_cJSON_GetObjectString(item, "image_url3", pitem[i].image_url3);
			pitem[i].pic_id = divoom_nba_pic_lookup_id(pitem[i].team_name, team_type, 0);
			pitem[i].pic_id2 = divoom_nba_pic_lookup_id(pitem[i].team_name, team_type, 1);
			pitem[i].pic_id3 = divoom_nba_pic_lookup_id(pitem[i].team_name, team_type, 2);
		}
		else
		{
			pitem[i].pic_id = divoom_nba_pic_lookup_id(pitem[i].team_name, team_type, 0);
		}
    }
    divoom_cJSON_Delete(root);
    return pitem;
}

static int divoom_nba_pic_lookup_prefix(const divoom_nba_pic_id_item *table, int count, char *team_name)
{
	int i, flag_len;

	for (i = 0; i < count; i++)
	{
		flag_len = (int)strlen(table[i].pic_flag);
		if (flag_len > 0 && memcmp(team_name, table[i].pic_flag, flag_len) == 0)
			return table[i].pic_id;
	}
	return -1;
}

static int divoom_nba_pic_lookup_exact(const divoom_nba_pic_id_item *table, int count, char *team_name)
{
	int i;

	for (i = 0; i < count; i++)
	{
		if (strcmp(team_name, table[i].pic_flag) == 0)
			return table[i].pic_id;
	}
	return -1;
}

#define DIVOOM_NBA_PIC_LIST_REQUEST_COOLDOWN  300

static uint8 team_list_request_pending[9];
static int team_list_request_time[9];

static int divoom_nba_pic_lookup_id(char *team_name, int team_type, int nba_flag)
{
	int i = 0;
	int image_id = 0;

	if (team_type == 0)
	{
		if (nba_flag == 0)
		{
			for (i = 0; i < sizeof(divoom_nba3_pic_table)/sizeof(divoom_nba3_pic_table[0]); i++)
			{
				if (memcmp(team_name, divoom_nba3_pic_table[i].pic_flag, 3) == 0)
					image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + divoom_nba3_pic_table[i].pic_id;
			}
		}
		else if (nba_flag == 1)
		{
			for (i = 0; i < sizeof(divoom_nba_pic_table)/sizeof(divoom_nba_pic_table[0]); i++)
			{
				if (memcmp(team_name, divoom_nba_pic_table[i].pic_flag, 3) == 0)
					image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + divoom_nba_pic_table[i].pic_id;
			}
		}
		else
		{
			for (i = 0; i < sizeof(divoom_nba2_pic_table)/sizeof(divoom_nba2_pic_table[0]); i++)
			{
				if (memcmp(team_name, divoom_nba2_pic_table[i].pic_flag, 3) == 0)
					image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + divoom_nba2_pic_table[i].pic_id;
			}
		}
	}
	else if (team_type == 1)
	{
		for (i = 0; i < sizeof(divoom_euro_cup_pic_table)/sizeof(divoom_euro_cup_pic_table[0]); i++)
		{
			if (memcmp(team_name, divoom_euro_cup_pic_table[i].pic_flag, 3) == 0)
				image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + divoom_euro_cup_pic_table[i].pic_id;
		}
	}
	else if (team_type == 2)
	{
		for (i = 0; i < sizeof(divoom_nfl_pic_table)/sizeof(divoom_nfl_pic_table[0]); i++)
		{
			if (memcmp(team_name, divoom_nfl_pic_table[i].pic_flag, 3) == 0)
				image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + divoom_nfl_pic_table[i].pic_id;
		}
	}
	else if (team_type == 3)
	{
		int pic_id;

		pic_id = divoom_nba_pic_lookup_prefix(divoom_premier_league_pic_table,
			sizeof(divoom_premier_league_pic_table) / sizeof(divoom_premier_league_pic_table[0]), team_name);
		if (pic_id < 0)
		{
			pic_id = divoom_nba_pic_lookup_exact(divoom_premier_league_abbrev_table,
				sizeof(divoom_premier_league_abbrev_table) / sizeof(divoom_premier_league_abbrev_table[0]), team_name);
		}
		if (pic_id >= 0)
			image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + pic_id;
	}
	else if (team_type == 4)
	{
		int pic_id;

		pic_id = divoom_nba_pic_lookup_prefix(divoom_laliga_pic_table,
			sizeof(divoom_laliga_pic_table) / sizeof(divoom_laliga_pic_table[0]), team_name);
		if (pic_id < 0)
		{
			pic_id = divoom_nba_pic_lookup_exact(divoom_laliga_abbrev_table,
				sizeof(divoom_laliga_abbrev_table) / sizeof(divoom_laliga_abbrev_table[0]), team_name);
		}
		if (pic_id >= 0)
			image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + pic_id;
	}
	else if (team_type == 5)
	{
		int pic_id;

		pic_id = divoom_nba_pic_lookup_prefix(divoom_seriea_pic_table,
			sizeof(divoom_seriea_pic_table) / sizeof(divoom_seriea_pic_table[0]), team_name);
		if (pic_id < 0)
		{
			pic_id = divoom_nba_pic_lookup_exact(divoom_seriea_abbrev_table,
				sizeof(divoom_seriea_abbrev_table) / sizeof(divoom_seriea_abbrev_table[0]), team_name);
		}
		if (pic_id >= 0)
			image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + pic_id;
	}
	else if (team_type == 6)
	{
		int pic_id;

		pic_id = divoom_nba_pic_lookup_prefix(divoom_bundesliga_pic_table,
			sizeof(divoom_bundesliga_pic_table) / sizeof(divoom_bundesliga_pic_table[0]), team_name);
		if (pic_id < 0)
		{
			pic_id = divoom_nba_pic_lookup_exact(divoom_bundesliga_abbrev_table,
				sizeof(divoom_bundesliga_abbrev_table) / sizeof(divoom_bundesliga_abbrev_table[0]), team_name);
		}
		if (pic_id >= 0)
			image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + pic_id;
	}
	else if (team_type == 7)
	{
		for (i = 0; i < sizeof(divoom_mlb_pic_table)/sizeof(divoom_mlb_pic_table[0]); i++)
		{
			if (memcmp(team_name, divoom_mlb_pic_table[i].pic_flag, strlen(divoom_mlb_pic_table[i].pic_flag)) == 0)
				image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + divoom_mlb_pic_table[i].pic_id;
		}
	}
	else if (team_type == 8)
	{
		for (i = 0; i < sizeof(divoom_afl_pic_table)/sizeof(divoom_afl_pic_table[0]); i++)
		{
			if (memcmp(team_name, divoom_afl_pic_table[i].pic_flag, strlen(divoom_afl_pic_table[i].pic_flag)) == 0)
				image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + divoom_afl_pic_table[i].pic_id;
		}
	}

	return image_id;
}

static void divoom_nba_pic_mark_team_list_requested(int team_type)
{
	int now;

	if (team_type < 0 || team_type >= 9)
		return;

	now = divoom_system_get_ticks();
	team_list_request_pending[team_type] = 1;
	team_list_request_time[team_type] = now;
}

static void divoom_nba_pic_clear_team_list_request(int team_type)
{
	if (team_type >= 0 && team_type < 9)
		team_list_request_pending[team_type] = 0;
}

static int divoom_nba_pic_queue_download(int image_id, char *url)
{
	char file_name[128];
	divoom_manage_image_item_info image_item;

	if (image_id <= 0 || url == NULL || url[0] == 0)
		return 0;

	divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, image_id, file_name);
	if (divoom_port_file_exists(file_name))
		return 1;

	image_item.flag = 1;
	image_item.image_id = image_id;
	strcpy(image_item.file_addr, url);
	image_item.pcallback = divoom_app_com_finishcallback;
	divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_item, 1);
	return 1;
}

static int divoom_nba_pic_try_download_one(char *team_name, int team_type, int nba_flag, int image_id)
{
	int i, num = 0;
	char *url = NULL;
	Pdivoom_team_img_item plist;

	plist = divoom_nba_pic_load(&num, team_type);
	if (plist == NULL || num <= 0)
		return 0;

	for (i = 0; i < num; i++)
	{
		if (strcmp(team_name, plist[i].team_name) != 0)
			continue;

		if (team_type == 0)
		{
			if (nba_flag == 0)
				url = plist[i].image_url1;
			else if (nba_flag == 1)
				url = plist[i].image_url2;
			else
				url = plist[i].image_url3;
		}
		else
			url = plist[i].image_url1;
		break;
	}
	divoom_system_free(plist);

	if (url == NULL || url[0] == 0)
		return 0;

	return divoom_nba_pic_queue_download(image_id, url);
}

static void divoom_nba_pic_request_team_list_if_needed(int team_type)
{
	int now;

	if (team_type < 0 || team_type >= 9)
		return;

	now = divoom_system_get_ticks();
	if (team_list_request_pending[team_type])
	{
		if (now - team_list_request_time[team_type] < DIVOOM_NBA_PIC_LIST_REQUEST_COOLDOWN)
			return;
	}

	divoom_nba_pic_mark_team_list_requested(team_type);
	divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALL_TEAM_IMG_LIST, DIVOOM_SERVER_TOPIC, &team_type);
}

void divoom_nba_pic_register_local_list(int team_type)
{
	int i, num = 0;
	Pdivoom_team_img_item plist;

	plist = divoom_nba_pic_load(&num, team_type);
	if (plist == NULL || num <= 0)
		return;

	for (i = 0; i < num; i++)
	{
		if (team_type == 0)
		{
			divoom_nba_pic_queue_download(plist[i].pic_id, plist[i].image_url1);
			divoom_nba_pic_queue_download(plist[i].pic_id2, plist[i].image_url2);
			divoom_nba_pic_queue_download(plist[i].pic_id3, plist[i].image_url3);
		}
		else
			divoom_nba_pic_queue_download(plist[i].pic_id, plist[i].image_url1);
	}
	divoom_system_free(plist);
}

void divoom_nba_pic_get_all_list(void)
{
	//如果不存在，将再次加载
	int team_type, i = 0;

	if (divoom_nba_pic_check_team_file_exist(0) == 0)
	{
		team_type = 0;
		divoom_nba_pic_mark_team_list_requested(team_type);
		divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALL_TEAM_IMG_LIST, DIVOOM_SERVER_TOPIC, &team_type);
	}
	else
		divoom_nba_pic_register_local_list(0);
    #if 0
	if (divoom_nba_pic_check_team_file_exist(2) == 0)
	{
		team_type = 2;
		divoom_nba_pic_mark_team_list_requested(team_type);
		divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALL_TEAM_IMG_LIST, DIVOOM_SERVER_TOPIC, &team_type);
	}
	else
		divoom_nba_pic_register_local_list(2);
    #endif
	if (divoom_nba_pic_check_team_file_exist(1) == 0)
	{
		team_type = 1;
		divoom_nba_pic_mark_team_list_requested(team_type);
		divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALL_TEAM_IMG_LIST, DIVOOM_SERVER_TOPIC, &team_type);
	}
	else
		divoom_nba_pic_register_local_list(1);
	if (divoom_nba_pic_check_team_file_exist(3) == 0) //英超
	{
		team_type = 3;
		divoom_nba_pic_mark_team_list_requested(team_type);
		divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALL_TEAM_IMG_LIST, DIVOOM_SERVER_TOPIC, &team_type);
	}
	else
		divoom_nba_pic_register_local_list(3);
	if (divoom_nba_pic_check_team_file_exist(4) == 0) //西甲
	{
		team_type = 4;
		divoom_nba_pic_mark_team_list_requested(team_type);
		divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALL_TEAM_IMG_LIST, DIVOOM_SERVER_TOPIC, &team_type);
	}
	else
		divoom_nba_pic_register_local_list(4);
	if (divoom_nba_pic_check_team_file_exist(5) == 0) //意甲
	{
		team_type = 5;
		divoom_nba_pic_mark_team_list_requested(team_type);
		divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALL_TEAM_IMG_LIST, DIVOOM_SERVER_TOPIC, &team_type);
	}
	else
		divoom_nba_pic_register_local_list(5);
	if (divoom_nba_pic_check_team_file_exist(6) == 0) //德甲
	{
		team_type = 6;
		divoom_nba_pic_mark_team_list_requested(team_type);
		divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALL_TEAM_IMG_LIST, DIVOOM_SERVER_TOPIC, &team_type);
	}
	else
		divoom_nba_pic_register_local_list(6);
	if (divoom_nba_pic_check_team_file_exist(7) == 0) //美国棒球大联盟
	{
		team_type = 7;
		divoom_nba_pic_mark_team_list_requested(team_type);
		divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALL_TEAM_IMG_LIST, DIVOOM_SERVER_TOPIC, &team_type);
	}
	else
		divoom_nba_pic_register_local_list(7);
	if (divoom_nba_pic_check_team_file_exist(8) == 0) //澳大利亚橄榄球
	{
		team_type = 8;
		divoom_nba_pic_mark_team_list_requested(team_type);
		divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALL_TEAM_IMG_LIST, DIVOOM_SERVER_TOPIC, &team_type);
	}
	else
		divoom_nba_pic_register_local_list(8);

}


/*********************************************************************
  Function:        divoom_nba_pic_update_list
  Description:     更新赛事列表
  Input:           plist
                   num
                    
  Return:          NONE
**********************************************************************/

void divoom_nba_pic_update_list(Pdivoom_team_img_item plist, int num, int team_type)
{
    int i, j, update_flag = 0, cur_number = 0, last_number;
    uint32 image_id;
    Pdivoom_team_img_item pcur_list, pnew_list;
    divoom_manage_image_item_info image_item;
	
    if(plist == NULL || num <= 0)
    {
        return;
    }
	divoom_nba_pic_clear_team_list_request(team_type);
    pcur_list = divoom_nba_pic_load(&cur_number, team_type);
    pnew_list = divoom_system_malloc(sizeof(divoom_team_img_item) * (cur_number + num));
    if(pnew_list == NULL)
    {
        divoom_system_free(pcur_list);
        return;
    }
    if (pcur_list)
    {
        memcpy(pnew_list, pcur_list, sizeof(divoom_team_img_item) *cur_number);
        divoom_system_free(pcur_list);
    }
    last_number = cur_number;
    for (i = 0; i < num; i ++)
    {
        for (j = 0; j < cur_number; j++)
        {
			if (strcmp(plist[i].team_name, pnew_list[j].team_name) == 0)
            {
				if (team_type == 0)
				{
	                if (strcmp(plist[i].image_url1, pnew_list[j].image_url1))
	                {
	                    //更新配置，先删除原来的文件，再添加下载新文件
	                    image_id = divoom_nba_pic_lookup_id(plist[i].team_name, team_type, 0);
	                    divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_id, 1);
	                    strcpy(pnew_list[j].image_url1, plist[i].image_url1);
	                    image_item.flag = 1;
	                    image_item.image_id = image_id;
	                    strcpy(image_item.file_addr, plist[i].image_url1);
	                    image_item.pcallback = divoom_app_com_finishcallback;
	                    divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_item, 1);
	                    update_flag = 1;
	                }
					if (strcmp(plist[i].image_url2, pnew_list[j].image_url2))
					{
						//更新配置，先删除原来的文件，再添加下载新文件
						image_id = divoom_nba_pic_lookup_id(plist[i].team_name, team_type, 1);
						divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_id, 1);
						strcpy(pnew_list[j].image_url2, plist[i].image_url2);
						image_item.flag = 1;
						image_item.image_id = image_id;
						strcpy(image_item.file_addr, plist[i].image_url2);
						image_item.pcallback = divoom_app_com_finishcallback;
						divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_item, 1);
						update_flag = 1;
					}

					if (strcmp(plist[i].image_url3, pnew_list[j].image_url3))
	                {
	                    //更新配置，先删除原来的文件，再添加下载新文件
	                    image_id = divoom_nba_pic_lookup_id(plist[i].team_name, team_type, 2);
	                    divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_id, 1);
	                    strcpy(pnew_list[j].image_url3, plist[i].image_url3);
	                    image_item.flag = 1;
	                    image_item.image_id = image_id;
	                    strcpy(image_item.file_addr, plist[i].image_url3);
	                    image_item.pcallback = divoom_app_com_finishcallback;
	                    divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_item, 1);
	                    update_flag = 1;
	                }
				}
				else 
				{
					if (strcmp(plist[i].image_url1, pnew_list[j].image_url1))
	                {
	                    //更新配置，先删除原来的文件，再添加下载新文件
	                    image_id = divoom_nba_pic_lookup_id(plist[i].team_name, team_type, 0);
	                    divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_id, 1);
	                    strcpy(pnew_list[j].image_url1, plist[i].image_url1);
	                    image_item.flag = 1;
	                    image_item.image_id = image_id;
	                    strcpy(image_item.file_addr, plist[i].image_url1);
	                    image_item.pcallback = divoom_app_com_finishcallback;
	                    divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_item, 1);
	                    update_flag = 1;
	                }
				}		
                break;
            }
        }
        if(j >= cur_number)
        {
            //增加下载项
			if (team_type == 0)
			{
	            image_item.flag = 1;
	            image_item.image_id = divoom_nba_pic_lookup_id(plist[i].team_name, team_type, 0);	
				plist[i].pic_id = image_item.image_id;
	            strcpy(image_item.file_addr, plist[i].image_url1);
	            image_item.pcallback = divoom_app_com_finishcallback;
#ifdef DOWNLOAD_TEST
				divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_item.image_id, 1);
#endif
	            divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_item, 1);

				image_item.image_id = divoom_nba_pic_lookup_id(plist[i].team_name, team_type, 1);
				plist[i].pic_id2 = image_item.image_id;
#ifdef DOWNLOAD_TEST
				divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_item.image_id, 1);
#endif
				strcpy(image_item.file_addr, plist[i].image_url2);
				divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_item, 1);

				image_item.image_id = divoom_nba_pic_lookup_id(plist[i].team_name, team_type, 2);
				plist[i].pic_id3 = image_item.image_id;
#ifdef DOWNLOAD_TEST
				divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_item.image_id, 1);
#endif
	            strcpy(image_item.file_addr, plist[i].image_url3);
	            divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_item, 1);
	            memcpy(&pnew_list[last_number], &plist[i], sizeof(divoom_team_img_item));
	            update_flag = 1;
				last_number++;
			}
			else
			{
	            image_item.flag = 1;
	            image_item.image_id = divoom_nba_pic_lookup_id(plist[i].team_name, team_type, 0);
				plist[i].pic_id = image_item.image_id;
#ifdef DOWNLOAD_TEST
				divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_item.image_id, 1);
#endif
    			strcpy(image_item.file_addr, plist[i].image_url1);
	            image_item.pcallback = divoom_app_com_finishcallback;
	            divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_item, 1);
				memcpy(&pnew_list[last_number], &plist[i], sizeof(divoom_team_img_item));
				update_flag = 1;
				last_number++;
			}
        }
    }

    if(update_flag)
    {
        divoom_nba_pic_save(pnew_list, last_number, team_type);
    }
    divoom_system_free(pnew_list);
}

/*********************************************************************
  Function:        divoom_nba_pic_get_id
  Description:     得到nba图片id
  Input:           team_name  队伍简称
                    team_type 赛事类型 0 nba 1 欧洲杯 2 NFL 3 英超 4 西甲 5 意甲 6 德甲 7 MLB
					nba_flag 0 赛程 1 实况 2 排名 
  Return:          图片ＩＤ
**********************************************************************/

int divoom_nba_pic_get_id(char *team_name, int team_type, int nba_flag)
{
	int image_id;
	char file_name[128] = {0};

	image_id = divoom_nba_pic_lookup_id(team_name, team_type, nba_flag);
	if (image_id == 0)
		return 0;

	divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, image_id, file_name);
	if (divoom_port_file_exists((char *)file_name))
		return image_id;

	if (!divoom_nba_pic_try_download_one(team_name, team_type, nba_flag, image_id))
		divoom_nba_pic_request_team_list_if_needed(team_type);

	return image_id;
}

static int divoom_nba_pic_copy_abbrev_from_tables(const divoom_nba_pic_id_item *pic_table, int pic_count,
	const divoom_nba_pic_id_item *abbrev_table, int abbrev_count, const char *team_name, char *out, int out_size)
{
	int i, j, flag_len, matched_pic_id = -1;

	if (team_name == NULL || out == NULL || out_size <= 0)
		return 0;

	for (i = 0; i < pic_count; i++)
	{
		if (strcmp(team_name, pic_table[i].pic_flag) == 0)
		{
			matched_pic_id = pic_table[i].pic_id;
			break;
		}
		flag_len = (int)strlen(pic_table[i].pic_flag);
		if (flag_len > 0 && strncmp(team_name, pic_table[i].pic_flag, flag_len) == 0)
		{
			matched_pic_id = pic_table[i].pic_id;
			break;
		}
	}

	if (matched_pic_id < 0)
	{
		for (i = 0; i < abbrev_count; i++)
		{
			if (strcmp(team_name, abbrev_table[i].pic_flag) == 0)
			{
				strncpy(out, abbrev_table[i].pic_flag, out_size - 1);
				out[out_size - 1] = '\0';
				return 1;
			}
		}
		return 0;
	}

	for (j = 0; j < abbrev_count; j++)
	{
		if (abbrev_table[j].pic_id == matched_pic_id)
		{
			strncpy(out, abbrev_table[j].pic_flag, out_size - 1);
			out[out_size - 1] = '\0';
			return 1;
		}
	}
	return 0;
}

int divoom_nba_pic_get_abbrev(const char *team_name, int team_type, char *out, int out_size)
{
	if (team_type == 3)
	{
		return divoom_nba_pic_copy_abbrev_from_tables(divoom_premier_league_pic_table,
			sizeof(divoom_premier_league_pic_table) / sizeof(divoom_premier_league_pic_table[0]),
			divoom_premier_league_abbrev_table,
			sizeof(divoom_premier_league_abbrev_table) / sizeof(divoom_premier_league_abbrev_table[0]),
			team_name, out, out_size);
	}
	if (team_type == 4)
	{
		return divoom_nba_pic_copy_abbrev_from_tables(divoom_laliga_pic_table,
			sizeof(divoom_laliga_pic_table) / sizeof(divoom_laliga_pic_table[0]),
			divoom_laliga_abbrev_table,
			sizeof(divoom_laliga_abbrev_table) / sizeof(divoom_laliga_abbrev_table[0]),
			team_name, out, out_size);
	}
	if (team_type == 5)
	{
		return divoom_nba_pic_copy_abbrev_from_tables(divoom_seriea_pic_table,
			sizeof(divoom_seriea_pic_table) / sizeof(divoom_seriea_pic_table[0]),
			divoom_seriea_abbrev_table,
			sizeof(divoom_seriea_abbrev_table) / sizeof(divoom_seriea_abbrev_table[0]),
			team_name, out, out_size);
	}
	if (team_type == 6)
	{
		return divoom_nba_pic_copy_abbrev_from_tables(divoom_bundesliga_pic_table,
			sizeof(divoom_bundesliga_pic_table) / sizeof(divoom_bundesliga_pic_table[0]),
			divoom_bundesliga_abbrev_table,
			sizeof(divoom_bundesliga_abbrev_table) / sizeof(divoom_bundesliga_abbrev_table[0]),
			team_name, out, out_size);
	}
	return 0;
}
