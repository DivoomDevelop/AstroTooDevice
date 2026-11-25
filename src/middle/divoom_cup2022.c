#include "divoom.h"

enum{
    DIVOOM_CUP2022_TREAM_UNACTIVE,  //没有开赛的
    DIVOOM_CUP2022_TREAM_ACTIVE,
    DIVOOM_CUP2022_TREAM_FINISH,
};
typedef struct _divoom_cup2022_team_name
{
    int image_id;
    char group_id[4];
    char team_id[16];
    char flag[4];
    char name[32];
}divoom_cup2022_team_name, *Pdivoom_cup2022_team_name;

static const divoom_cup2022_team_name cup2022_team_list[] =
{
    {DIVOOM_DISP_EXCHANGE2_SEN, "A3","塞内加尔", "SEN", "Senegal"},
    {DIVOOM_DISP_EXCHANGE2_NED, "A4","荷兰", "NED", "Nederlands"},
    {DIVOOM_DISP_EXCHANGE2_ENG, "B1","英格兰", "ENG", "England"},
    {DIVOOM_DISP_EXCHANGE2_IRI, "B2","伊朗", "IRI", "Iran"},
    {DIVOOM_DISP_EXCHANGE2_QAT, "A1","卡塔尔", "QAT", "Qatar"},
    {DIVOOM_DISP_EXCHANGE2_ECU, "A2","厄瓜多尔", "ECU", "Ecuador"},
    {DIVOOM_DISP_EXCHANGE_USD, "B3","美国", "USA", "United States"},
    {DIVOOM_DISP_EXCHANGE2_WAL, "B4","威尔士", "WAL", "Wales"},
    {DIVOOM_DISP_EXCHANGE2_ARG, "C1","阿根廷","ARG", "Argentina"},
    {DIVOOM_DISP_EXCHANGE2_KSA, "C2","沙特阿拉伯","KSA", "Saudi Arabia"},
    {DIVOOM_DISP_EXCHANGE2_DEN, "D3","丹麦","DEN", "Denmark"},
    {DIVOOM_DISP_EXCHANGE2_TUN, "D4","突尼斯","TUN", "Tunisia"},
    {DIVOOM_DISP_EXCHANGE2_MEX, "C3","墨西哥","MEX", "Mexico"},
    {DIVOOM_DISP_EXCHANGE2_POL, "C4","波兰","POL", "Poland"},
    {DIVOOM_DISP_EXCHANGE2_FRA, "D1","法国","FRA", "France"},
    {DIVOOM_DISP_EXCHANGE2_AUS, "D2","澳大利亚","AUS", "Australia"},
    {DIVOOM_DISP_EXCHANGE2_MAR, "F1","摩洛哥","MAR", "Morocco"},
    {DIVOOM_DISP_EXCHANGE2_CRO, "F2","克罗地亚","CRO", "Croatia"},
    {DIVOOM_DISP_EXCHANGE2_GER, "E3","德国","GER", "Germany"},
    {DIVOOM_DISP_EXCHANGE_JPY, "E4","日本","JPY", "Japan"},
    {DIVOOM_DISP_EXCHANGE2_ESP, "E1","西班牙","ESP", "Spain"},
    {DIVOOM_DISP_EXCHANGE2_CRC, "E2","哥斯达黎加","CRC", "Costa Rica"},
    {DIVOOM_DISP_EXCHANGE2_BEL, "F3","比利时","BEL", "Belgium"},
    {DIVOOM_DISP_EXCHANGE_CAD, "F4","加拿大","CAN", "Canada"},
    {DIVOOM_DISP_EXCHANGE_BRL, "G1","巴西","BRA", "Brazil"},
    {DIVOOM_DISP_EXCHANGE_SRB, "G2","塞尔维亚","SRB", "Serbia"},
    {DIVOOM_DISP_EXCHANGE2_POR, "H3","葡萄牙","POR", "Portugal"},
    {DIVOOM_DISP_EXCHANGE2_GHA, "H4","加纳","GHA", "Ghana"},
    {DIVOOM_DISP_EXCHANGE2_URN, "H1","乌拉圭","URU", "Uruguay"},
    {DIVOOM_DISP_EXCHANGE_KRW, "H2","韩国","KOR", "South Korea"},
    {DIVOOM_DISP_EXCHANGE_CHF, "G3","瑞士","SUI", "Switzerland"},
    {DIVOOM_DISP_EXCHANGE2_CMR, "G4","喀麦隆","CMR", "Cameroon"},
};
typedef struct _divoom_cup2022_item
{
    int  group_id;
    char  away_team_name[4];
    char  home_team_name[4];
    int away_image_id;   //队对应的图片
    int home_image_id;   //队对应的图片
    int away_score;
    int home_score;
    int play_utc_time;
}divoom_cup2022_item, *Pdivoom_cup2022_item;


typedef struct _divoom_cup2022_team_info
{
    char    team_name[4];
    int     group_id;
    int     image_id;   //队对应的图片
    int     score;
}divoom_cup2022_team_info, *Pdivoom_cup2022_team_info;

typedef struct _divoom_cup2022_ctrl
{
    divoom_http_client_url_head_info url_head;
    int update_token;
    int request_type;
    int team_num;
    int request_time;
    int run_num;
    Pdivoom_cup2022_item pteam_list;
    int group_num;
    divoom_cup2022_team_info pgroup_list[8][4];
    divoom_cup2022_item runing_team[4];
}divoom_cup2022_ctrl, *Pdivoom_cup2022_ctrl;


Pdivoom_cup2022_ctrl pdivoom_cup2022_handle = NULL;

void divoom_cup2022_init(void)
{
    if(pdivoom_cup2022_handle)
    {
        return;
    }
    pdivoom_cup2022_handle = divoom_system_malloc(sizeof(divoom_cup2022_ctrl));
    pdivoom_cup2022_handle->team_num = 0;
    pdivoom_cup2022_handle->pteam_list = NULL;
    pdivoom_cup2022_handle->request_type = -1;
}

void divoom_cup2022_load_score_list(Pdivoom_http_client_url_head_info phead_info, int head_num, int cur_time, void *phandle)
{
    int i, j, k,group_id, team_id = 0, num, status;
    char *ret_data, tmp_str[64];
 	divoom_cJSON *root = NULL, *list, *item, *team_list, *team_item;


    ret_data = divoom_http_client_url_request("https://www.douyin.com/aweme/v1/web/backpack/team/ranking", NULL, 0, divoom_clock_control_check_exit_flag, phandle);
    if(ret_data == NULL)
    {
        divoom_print("err!\n");
        return;
    }
	root = divoom_cJSON_Parse(ret_data);
    divoom_system_free(ret_data);
    if(root == NULL)
    {
        divoom_print("err!\n");
        return;
    }
    status  = divoom_cJSON_GetObjectInt(root, "status_code");
    if(status != 0)
    {
        divoom_cJSON_Delete(root);
        return;
    }
    
    list = divoom_cJSON_GetObjectItem(divoom_cJSON_GetObjectItem(root, "sections"), "ranking_groups");
    if(list == NULL || divoom_cJSON_GetArraySize(list)<=0)
    {
        divoom_cJSON_Delete(root);
        divoom_print("err!\n");
        return;
    }    
    num = divoom_cJSON_GetArraySize(list); 
    
    for(i = 0; i < num; i ++)
    {
        item = divoom_cJSON_GetArrayItem(list, i);
        if(item == NULL)
        {
            divoom_print("err!\n");
            break;
        }
        
        divoom_cJSON_GetObjectString(item, "group_name", tmp_str);
        if(strlen(tmp_str)==0)
        {
            continue;
        }
        group_id = tmp_str[0] - 'A';
        if(group_id >=8)
        {
            divoom_print("err!\n");
            continue;
        }
        team_list = divoom_cJSON_GetObjectItem(item, "standings");
        for(j = 0; j < divoom_cJSON_GetArraySize(team_list); j++)
        {
            team_item = divoom_cJSON_GetArrayItem(team_list, j);
            if(team_item == NULL)
            {
                divoom_print("err!\n");
                break;
            }
            divoom_cJSON_GetObjectString(team_item, "team_name", tmp_str);
            for(k = 0; k < sizeof(cup2022_team_list)/sizeof(cup2022_team_list[0]); k++)
            {
                if(strcmp(tmp_str, cup2022_team_list[k].team_id) == 0)
                {
                    team_id = cup2022_team_list[k].group_id[1] - '1';
                    pdivoom_cup2022_handle->pgroup_list[group_id][team_id].image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + cup2022_team_list[k].image_id;
                    strcpy(pdivoom_cup2022_handle->pgroup_list[group_id][team_id].team_name, cup2022_team_list[k].flag);
                    break;
                }
            }
            if(k >= sizeof(cup2022_team_list)/sizeof(cup2022_team_list[0]))
            {
                divoom_print("err find: %s\n", tmp_str);
                continue;
            }
            
            pdivoom_cup2022_handle->pgroup_list[group_id][team_id].score = divoom_cJSON_GetObjectInt(team_item, "points");
            divoom_print("team info: %d, %d, %c%c, score: %d, name: %s\n",group_id,team_id,group_id+'A',team_id+'1', pdivoom_cup2022_handle->pgroup_list[group_id][team_id].score, pdivoom_cup2022_handle->pgroup_list[group_id][team_id].team_name);
        }
    } 
    divoom_cJSON_Delete(root);
}

void divoom_cup2022_get_score_item(divoom_clock_control_disp_array_item *pdisp_item, int group_id)
{
    int i, j, pass_flag[4] = {0}, find_pos[4] ={0}, max_pos = -1;

    for(i = 0; i <4; i ++)
    {
        for(j = 0, max_pos = -1; j < 4; j ++)
        {
            if(pass_flag[j])
            {
                continue;
            }
            if(max_pos == -1)
            {
                max_pos = j;
            }
            else if(pdivoom_cup2022_handle->pgroup_list[group_id][j].score > pdivoom_cup2022_handle->pgroup_list[group_id][max_pos].score)
            {
                max_pos = j;
            }
        }
        if(max_pos != -1)
        {
            pass_flag[max_pos] = 1;
            find_pos[i] = max_pos;
        }
        else
        {
        }
    }
    pdisp_item->image_id = pdivoom_cup2022_handle->pgroup_list[group_id][find_pos[0]].image_id;
    pdisp_item->image_id2 = pdivoom_cup2022_handle->pgroup_list[group_id][find_pos[1]].image_id;
    pdisp_item->image_id3 = pdivoom_cup2022_handle->pgroup_list[group_id][find_pos[2]].image_id;
    pdisp_item->image_id4 = pdivoom_cup2022_handle->pgroup_list[group_id][find_pos[3]].image_id;
    sprintf(pdisp_item->pdisp_data[0], "%c%c", group_id + 'A',pdivoom_cup2022_handle->pgroup_list[group_id][find_pos[0]].group_id+'1');
    sprintf(pdisp_item->pdisp_data[1], "%c%c", group_id + 'A',pdivoom_cup2022_handle->pgroup_list[group_id][find_pos[1]].group_id+'1');
    sprintf(pdisp_item->pdisp_data[2], "%02d", pdivoom_cup2022_handle->pgroup_list[group_id][find_pos[0]].score);
    sprintf(pdisp_item->pdisp_data[3], "%02d", pdivoom_cup2022_handle->pgroup_list[group_id][find_pos[1]].score);
    strcpy(pdisp_item->pdisp_data[4], pdivoom_cup2022_handle->pgroup_list[group_id][find_pos[0]].team_name);
    strcpy(pdisp_item->pdisp_data[5], pdivoom_cup2022_handle->pgroup_list[group_id][find_pos[1]].team_name);
    
    sprintf(pdisp_item->pdisp_data[6], "%c%c", group_id + 'A',pdivoom_cup2022_handle->pgroup_list[group_id][find_pos[2]].group_id+'1');
    sprintf(pdisp_item->pdisp_data[7], "%c%c", group_id + 'A',pdivoom_cup2022_handle->pgroup_list[group_id][find_pos[3]].group_id+'1');
    sprintf(pdisp_item->pdisp_data[8], "%02d", pdivoom_cup2022_handle->pgroup_list[group_id][find_pos[2]].score);
    sprintf(pdisp_item->pdisp_data[9], "%02d", pdivoom_cup2022_handle->pgroup_list[group_id][find_pos[3]].score);
    strcpy(pdisp_item->pdisp_data[10], pdivoom_cup2022_handle->pgroup_list[group_id][find_pos[2]].team_name);
    strcpy(pdisp_item->pdisp_data[11], pdivoom_cup2022_handle->pgroup_list[group_id][find_pos[3]].team_name);
    pdisp_item->num = 12;
}

uint8 divoom_cup2022_get_score(Pdivoom_clock_control_disp_data pdisp_data, Pdivoom_http_client_url_head_info phead_info, int head_num, void *phandle)
{
    int i;

    
    if(pdivoom_cup2022_handle == NULL)
    {        
        divoom_cup2022_init();
    }

    pdivoom_cup2022_handle->request_type = 1;
    divoom_cup2022_load_score_list(phead_info, head_num, 0, phandle);
    
    pdisp_data->num = 8;
    for(i = 0; i < 8; i ++)
    {
        
        divoom_cup2022_get_score_item(divoom_clock_control_disp_data_get_item(pdisp_data, i), i);
    }
    return 1;
}
#if 0
static const int all_start_time[] =
{   
    1669543200,
    1669554000,
    1669564800,
    1669575600,
    1669629600,
    1669640400,
    1669651200,
    1669662000,
    1669734000,
    1669734000,
    1669748400,
    1669748400,
    1669820400,
    1669820400,
    1669834800,
    1669834800,
    1669906800,
    1669906800,
    1669921200,
    1669921200,
    1669993200,
    1669993200,
    1670007600,
    1670007600,
    1670079600,
    1670094000,
    1670166000,
    1670180400,
    1670252400,
    1670266800,
    1670338800,
    1670353200,
    1670598000,
    1670612400,
    1670684400,
    1670698800,
    1670958000,
    1671044400,
    1671289200,
    1671375600,
};
static const char *cup2022_base_info  = "https://api.msn.com/sports/schedules?apikey=kO1dI4ptCTTylLkPL1ZTHYP8JhLKb8mRDoA5yotmNJ&version=1.0&market=zh-cn&tzoffset=8&date=%d-%d-%d&ids=Soccer_InternationalWorldCup&withcalendar=false&type=LeagueSchedule";

//如果有当前比赛的队就返回当前比赛的队，否则返回最近一场已经比赛的队
uint8 divoom_cup2022_analysis_real_data(divoom_cJSON *root, Pdivoom_clock_control_disp_data pdisp_data, int cur_time)
{
    int i, j, k, num, runing_num = 0, over_utc_time = 0, item_utc_time, score;
    char tmp_str[64], team_name[20];
 	divoom_cJSON *list, *item, *obj, *team_list, *team_item;
    divoom_cup2022_item runing_team[4], over_team, *pcur_team, new_team_info[4];
    
    obj =  divoom_cJSON_GetObjectItem(root, "value");
    if(obj == NULL)
    {
        divoom_print("aj err!\n");
        return 0;
    }
    obj =  divoom_cJSON_GetArrayItem(obj, 0);
    if(obj == NULL)
    {
        divoom_print("aj err!\n");
        return 0;
    }
    obj = divoom_cJSON_GetObjectItem(obj, "schedules");
    if(obj == NULL)
    {
        divoom_print("aj err!\n");
        return 0;
    }
    obj =  divoom_cJSON_GetArrayItem(obj, 0);
    if(obj == NULL)
    {
        divoom_print("aj err!\n");
        return 0;
    }
    list = divoom_cJSON_GetObjectItem(obj, "games");
    if(list == NULL)
    {
        divoom_print("aj err!\n");
        return 0;
    }
    num = divoom_cJSON_GetArraySize(list);
    for(i = 0; i < num; i++)
    {
        item = divoom_cJSON_GetArrayItem(list, i);
        if(item == NULL)
        {
            divoom_print("aj err!\n");
            break;
        }        
        divoom_cJSON_GetObjectString(item, "startDateTime", tmp_str);
        item_utc_time = atoi(tmp_str)/1000;
        obj = divoom_cJSON_GetObjectItem(item, "gameState");
        if(obj == NULL)
        {
            divoom_print("aj err!\n");
            continue;
        }
        divoom_cJSON_GetObjectString(obj, "gameStatus", tmp_str);
        if(strcmp(tmp_str, "PreGame") == 0)
        {
            //还没有开始的
            divoom_print("aj err!\n");
            continue;
        }
        else if(strcmp(tmp_str, "Final") == 0)
        {
            //已经结束的
            if(over_utc_time >= item_utc_time)
            {
                continue;
            }
            over_utc_time = item_utc_time;            
            pcur_team = &over_team;
            
        }
        else  if(strcmp(tmp_str, "InProgress") == 0)
        {
            //正在进行的
            pcur_team = &runing_team[runing_num];
            runing_num ++;
            divoom_cJSON_GetObjectString(divoom_cJSON_GetObjectItem(obj, "gameClock"), "minutes", tmp_str);
            pcur_team->play_utc_time = atoi(tmp_str) * 60;
            divoom_cJSON_GetObjectString(divoom_cJSON_GetObjectItem(obj, "gameClock"), "seconds", tmp_str);
            pcur_team->play_utc_time = +atoi(tmp_str);
        }
        else
        {
            continue;
        }
        team_list = divoom_cJSON_GetObjectItem(item, "participants");
        if(team_list == NULL)
        {
            divoom_print("aj err!\n");
            break;
        }
        for(j = 0; j < divoom_cJSON_GetArraySize(team_list); j++)
        {
            team_item = divoom_cJSON_GetArrayItem(team_list, j);
            if(team_item == NULL)
            {
                divoom_print("aj err!: %d,%d\n",j, divoom_cJSON_GetArraySize(team_list));
                break;
            }
            divoom_cJSON_GetObjectString(divoom_cJSON_GetObjectItem(divoom_cJSON_GetObjectItem(team_item, "team"), "name"), "rawName", team_name);
            for(k = 0; k < sizeof(cup2022_team_list)/sizeof(cup2022_team_list[0]); k++)
            {
                if(strcmp(cup2022_team_list[k].name, team_name)==0)
                {
                    break;
                }
            }
            if(k >= sizeof(cup2022_team_list)/sizeof(cup2022_team_list[0]))
            {
                divoom_print_err("we can't find team: %s\n", team_name);
                break;
            }
            obj = divoom_cJSON_GetObjectItem(team_item, "playingPeriodScores");
            if(obj == NULL)
            {
                divoom_print("aj err!\n");
                break;
            }
            divoom_cJSON_GetObjectString(divoom_cJSON_GetObjectItem(team_item, "result"), "score", tmp_str);
            score = atoi(tmp_str);
            divoom_print("score info: %s, %d\n", tmp_str, score);
            divoom_cJSON_GetObjectString(team_item, "homeAwayStatus", tmp_str);
            if(strcmp(tmp_str, "Away")==0)
            {
                pcur_team->away_score = score;
                pcur_team->away_image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + cup2022_team_list[k].image_id;
                strcpy(pcur_team->away_team_name, cup2022_team_list[k].flag);
                divoom_print("find: %s, %s, %d, %d\n", tmp_str, pcur_team->away_team_name, pcur_team->away_score, pcur_team->away_image_id);
            }
            else
            {
                pcur_team->home_score = score;
                pcur_team->home_image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + cup2022_team_list[k].image_id;
                strcpy(pcur_team->home_team_name, cup2022_team_list[k].flag);
                divoom_print("find: %s, %s, %d, %d\n", tmp_str, pcur_team->home_team_name, pcur_team->home_score, pcur_team->home_image_id);
            }
        }
    }
    divoom_print("result: %d, %d\n", runing_num, over_utc_time);
    if(runing_num)
    {
        //更新一下数据，如果发现数据过旧，将直接不用
        for(i = 0; i < runing_num; i++)
        {
            for(j = 0; j < pdivoom_cup2022_handle->run_num; j++)
            {
                if(strcmp(pdivoom_cup2022_handle->runing_team[j].away_team_name, runing_team[i].away_team_name) == 0 &&
                    strcmp(pdivoom_cup2022_handle->runing_team[j].home_team_name, runing_team[i].home_team_name) == 0 )
                {
                    if(pdivoom_cup2022_handle->runing_team[j].play_utc_time > runing_team[i].play_utc_time)
                    {
                        divoom_print("pass the request!%d, %d\n", pdivoom_cup2022_handle->runing_team[j].play_utc_time,runing_team[i].play_utc_time);
                        memcpy(&new_team_info[i], &pdivoom_cup2022_handle->runing_team[j], sizeof(divoom_cup2022_item));
                    }
                    else
                    {
                        memcpy(&new_team_info[i], &runing_team[i], sizeof(divoom_cup2022_item));
                        divoom_print("update the request!%d, %d\n", pdivoom_cup2022_handle->runing_team[j].play_utc_time, runing_team[i].play_utc_time);
                    }
                    break;
                }
            }
            if(j >= pdivoom_cup2022_handle->run_num)
            {
                memcpy(&new_team_info[i], &runing_team[i], sizeof(divoom_cup2022_item));
                divoom_print("update the request!%d,\n", runing_team[i].play_utc_time);
            }
        }
        pdivoom_cup2022_handle->run_num = runing_num;
        memcpy(pdivoom_cup2022_handle->runing_team, new_team_info, runing_num*sizeof(divoom_cup2022_item));
        
        
        for(i = 0; i < runing_num; i++)
        {
            DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).image_id = new_team_info[i].home_image_id;
            DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).image_id2 = new_team_info[i].away_image_id;
            sprintf(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).pdisp_data[0], "%02d",new_team_info[i].home_score);
            sprintf(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).pdisp_data[1], "%02d",new_team_info[i].away_score);
            sprintf(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).pdisp_data[2], "%s",new_team_info[i].home_team_name);
            sprintf(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).pdisp_data[3], "%s",new_team_info[i].away_team_name);
            DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, i).num = 4;
        }
        pdisp_data->num = runing_num;
    }
    else if(over_utc_time)
    {
        DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).image_id = over_team.home_image_id;
        DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).image_id2 = over_team.away_image_id;
        sprintf(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[0], "%02d",over_team.home_score);
        sprintf(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[1], "%02d",over_team.away_score);
        sprintf(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[2], "%s",over_team.home_team_name);
        sprintf(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).pdisp_data[3], "%s",over_team.away_team_name);
        DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, 0).num = 4;
        pdisp_data->num = 1;
    }
    else
    {
        divoom_print("no result!\n");
        return 0;
    }
    
    return 1;
}
#endif
uint8 divoom_cup2022_get_data(char *url_info, Pdivoom_clock_control_disp_data pdisp_data, Pdivoom_http_client_url_head_info phead_info, int head_num, void *pdivoom_clock_control_handle)
{
    int i, j, game_status;
    char *ret_data, game_name[64];
 	divoom_cJSON *root = NULL, *list, *item;

    //divoom_print("we will request!\n");
    ret_data = divoom_http_client_url_request("https://www.douyin.com/aweme/v1/web/backpack/match/list", NULL, 0, divoom_clock_control_check_exit_flag, pdivoom_clock_control_handle);
    if(ret_data == NULL)
    {
        divoom_print("aj err!\n");
        return 0;
    }
	root = divoom_cJSON_Parse(ret_data);
    divoom_system_free(ret_data);
    if(root == NULL)
    {
        divoom_print("aj err!\n");
        return 0;
    }
    list =  divoom_cJSON_GetObjectItem(root, "match_list");
    if(list == NULL)
    {
        divoom_cJSON_Delete(root);     
        divoom_print("aj err!\n");
        return 0;
    }
    pdisp_data->num = 0;
    for(i = 0; i < divoom_cJSON_GetArraySize(list); i++)
    {
        item =  divoom_cJSON_GetArrayItem(list, i);
        if(item == NULL)
        {
            divoom_print("aj err!\n");
            break;
        }
        game_status = divoom_cJSON_GetObjectInt(item, "match_status");
        if(game_status == 0)
        {
            break;
        }
        divoom_cJSON_GetObjectString(item, "host_name", game_name);
        for(j = 0; j < sizeof(cup2022_team_list)/sizeof(cup2022_team_list[0]); j++)
        {
            if(strcmp(game_name, cup2022_team_list[j].team_id) == 0)
            {
                //divoom_print("find: %s!\n", game_name);
                break;
            }
        }
        if(j < sizeof(cup2022_team_list)/sizeof(cup2022_team_list[0]))
        {
            DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, pdisp_data->num).image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + cup2022_team_list[j].image_id;
            sprintf(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, pdisp_data->num).pdisp_data[0], "%02d",divoom_cJSON_GetObjectInt(item, "host_score"));
            sprintf(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, pdisp_data->num).pdisp_data[2], "%s",cup2022_team_list[j].flag);
        }
        else
        {
            //divoom_print("find err home: %s\n", game_name);
        }
        
        divoom_cJSON_GetObjectString(item, "guest_name", game_name);
        for(j = 0; j < sizeof(cup2022_team_list)/sizeof(cup2022_team_list[0]); j++)
        {
            if(strcmp(game_name, cup2022_team_list[j].team_id) == 0)
            {
                //divoom_print("find: %s!\n", game_name);
                break;
            }
        }
        if(j < sizeof(cup2022_team_list)/sizeof(cup2022_team_list[0]))
        {
            DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, pdisp_data->num).image_id2 = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + cup2022_team_list[j].image_id;
            sprintf(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, pdisp_data->num).pdisp_data[1], "%02d",divoom_cJSON_GetObjectInt(item, "guest_score"));
            sprintf(DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, pdisp_data->num).pdisp_data[3], "%s",cup2022_team_list[j].flag);
        }
        else
        {
            //divoom_print("find err away: %s\n", game_name);
        }
        
        if(game_status == 1)
        {
            //divoom_print("find!\n");
            DIVOOM_CLOCK_CONTROL_DISP_DATA_ITEM(pdisp_data, pdisp_data->num).num = 4;
            pdisp_data->num ++;
        }
        
    }
    if(pdisp_data->num == 0)
    {
        pdisp_data->num = 1;
    }
    divoom_cJSON_Delete(root);     
    return 1;
}


