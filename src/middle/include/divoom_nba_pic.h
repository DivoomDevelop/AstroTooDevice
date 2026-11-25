#ifndef _divoom_nba_pic_
#define _divoom_nba_pic_

void divoom_nba_pic_get_all_list(void);
void divoom_nba_pic_update_list(Pdivoom_team_img_item plist, int num, int team_type);
int divoom_nba_pic_get_id(char *team_name, int team_type, int nba_flag);
int divoom_nba_pic_get_abbrev(const char *team_name, int team_type, char *out, int out_size);

#endif
