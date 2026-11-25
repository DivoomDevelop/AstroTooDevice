#ifndef _divoom_clock_notice_h_
#define _divoom_clock_notice_h_

enum{
    DIVOOM_CLOCK_NOTICE_GMAIL_TYPE = 1,             //GMAIL
    DIVOOM_CLOCK_NOTICE_HOME_CONNECCT_TYPE,     //HOME-CONNECT
    DIVOOM_CLOCK_NOTICE_GOOGLE_CALENDAR_TYPE,   //谷歌日历
    DIVOOM_CLOCK_NOTICE_TWITCH_AUTHOR_TYPE,     //TWITCH主播
    DIVOOM_CLOCK_NOTICE_TWITTER_AUTHOR_TYPE,    //TWITTER作者
    DIVOOM_CLOCK_NOTICE_DOUYIN_AUTHOR_TYPE,      //抖音作者
    DIVOOM_CLOCK_NOTICE_INSTAGRAM_AUTHOR_TYPE,    //INSTAGRAM作者
    DIVOOM_CLOCK_NOTICE_BTC_CLOCK_TYPE,             //btc
    DIVOOM_CLOCK_NOTICE_TWITTER_ARTICLE_TYPE,    //TWITTER文章
    DIVOOM_CLOCK_NOTICE_TITKOK_ARTICLE_TYPE,    //TIKTOK作品
    DIVOOM_CLOCK_NOTICE_YOUTUBE_CHANNEL_TIMEGATE_TYPE,      //YOUTUBE频道
    DIVOOM_CLOCK_NOTICE_YOUTUBE_VIDEO_TIMEGATE_TYPE,      //YOUTUBE视频
    DIVOOM_CLOCK_NOTICE_YOUTUBE_CHANNEL_TYPE,      //YOUTUBE频道
    DIVOOM_CLOCK_NOTICE_YOUTUBE_VIDEO_TYPE,      //YOUTUBE视频
    DIVOOM_CLOCK_NOTICE_FACEBOOK_VIDEO_TYPE,    //FACEBOOK作品
    DIVOOM_CLOCK_NOTICE_FACEBOOK_PHOTO_TYPE,    //FACEBOOK相片
    DIVOOM_CLOCK_NOTICE_INSTAGRAM_VIDEO_TYPE,    //INSTAGRAM视频
    DIVOOM_CLOCK_NOTICE_TWITCH_LIVE_ROOM_TYPE,  //TWITCH直播间
    DIVOOM_CLOCK_NOTICE_MAX_TYPE = 128,
};

typedef void divoom_clock_notice_active_clock(int clock_id, int lcd_id, int indenpence_index);

/*********************************************************************
  Function:         divoom_clock_notice_update
  Description:      更新表盘通知开关
  Input:            pclock_info
                    
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_clock_notice_update(Pdivoom_channel_clock_notify_list pinfo);


/*********************************************************************
  Function:         divoom_clock_notice_init
  Description:      表盘提醒模块初始化
  Input:            none
                    
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_clock_notice_init(Pdivoom_channel_clock_notify_list pinfo, divoom_clock_notice_active_clock *pntoice_call);

void divoom_clock_notice_main_loop(int cur_time);

#endif


