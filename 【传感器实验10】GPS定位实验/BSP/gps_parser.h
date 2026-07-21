#ifndef __GPS_PARSER_H__
#define __GPS_PARSER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 根据实际的数据修改ID
#define ID_GGA "$GNGGA"
#define ID_RMC "$GNRMC"
#define ID_GLL "$GNGLL"
#define ID_GSA "$GNGSA"
#define ID_GSV "$GPGSV"
#define ID_VTG "$GNVTG"

// 语句解析控制 1：使能，0：禁用
#define ENABLE_GGA 1
#define ENABLE_RMC 1
#define ENABLE_GLL 1
#define ENABLE_GSA 1
#define ENABLE_GSV 1
#define ENABLE_VTG 1
#define ENABLE_UTC 0

void gps_parse(char *gps_data);

#endif //__GPS_PARSER_H__

#if ENABLE_GGA
// GGA字段结构体（GPS定位数据）
typedef struct {
    char utc[11]; // UTC时间，格式为hhmmss.sss
    double lat; // 纬度，格式为ddmm.mmmm
    char lat_dir; // 纬度半球，N或S
    double lon; // 经度，格式为dddmm.mmmm
    char lon_dir; // 经度半球，E或W
    unsigned char quality; // 0=定位无效，1=定位有效
    unsigned char sats; // 使用卫星数量，从00到12
    double hdop; // 水平精确度，0.5到99.9，单位m
    double alt; // 海平面的高度，-9999.9到9999.9米
    double undulation; // 大地水准面高度，-9999.9到9999.9米
    unsigned char age; // 差分时间
    unsigned short stn_ID; // 差分站ID号0000-1023
} GGA;
void gga_parse(GGA *gga, char *gga_data);
void gga_show(GGA *gga);
#endif

#if ENABLE_RMC
//RMC数据结构体（推荐定位信息数据格式）
typedef struct {
    char utc[11];                     // UTC时间，hhmmss.sss格式
    unsigned char status;         // 状态，A=定位，V=未定位
    double lat;                       // 纬度ddmm.mmmm，度分格式
    char lat_dir;                     // 纬度N（北纬）或S（南纬）
    double lon;                       // 经度dddmm.mmmm，度分格式
    char lon_dir;                     // 经度E（东经）或W（西经）
    double speed_Kn;                  // 速度
    double track_true;                // 方位角，度
    char date[7];                     // UTC date日期，DDMMYY格式
    double mag_var;                   // 磁偏角，（000 - 180）度
    char mag_var_dir;                     // 磁偏角方向，E=东W=西
    char mode;       // 模式，A=自动，D=差分，E=估测，N=数据无效（3.0协议内容）
    char nav_status;   //导航状态，S=安全 , C=注意，U=不安全，V= 导航状态无效
}RMC;
void rmc_parse(RMC *rmc, char *rmc_str);
void rmc_show(RMC *rmc);
#endif

#if ENABLE_GLL
// GPGLL数据结构体（地理定位信息）
typedef struct {
    double lat;                       // 纬度，格式为ddmm.mmmm
    char lat_dir;                     // 纬度半球，N或S
    double lon;                       // 经度，格式为dddmm.mmmm
    char lon_dir;                     // 经度半球，E或W
    char utc[11];                     // UTC时间，格式为hhmmss.sss
    char status;                 // 状态标志位，A：有效，V无效
}GLL;
void gll_parse(GLL *gll, char *gll_str);
void gll_show(GLL *gll);
#endif

#if ENABLE_GSA
//GSA数据结构体（当前卫星信息）
typedef struct {
    unsigned char mode_MA;            // 定位模式(选择2D/3D)，A=自动选择，M=手动选择
    unsigned char mode_123;           // 定位类型，1=未定位，2=2D定位，3=3D定位
    unsigned char prn[12];
    double pdop;                      // PDOP综合位置精度因子（0.5 - 99.9）
    double hdop;                      // HDOP水平精度因子（0.5 - 99.9）
    double vdop;                      // VDOP垂直精度因子（0.5 - 99.9）
    unsigned char sysid;              // 定位系统类型
}GSA;
void gsa_parse(GSA *gsa, char *gsa_str);
void gsa_show(GSA *gsa);
#endif

#if ENABLE_GSV
#pragma pack(1)                       // 便于指针偏移取值
// 可见卫星信息结构体
typedef struct {
    unsigned char prn;                // PRN 码（伪随机噪声码）
    unsigned char elev;               // 卫星仰角（00 - 90）度
    unsigned short azimuth;           // 卫星方位角（00 - 359）度
    unsigned char SNR;                // 信噪比
}SAT_INFO;
#pragma pack()

// GSV数据结构体(可见卫星信息)
typedef struct {
    unsigned char  msgs;               // 本次GSV语句的总数目
    unsigned char  msg;               // 当前处理的是条目编号
    unsigned char sats;               // 当前可见卫星总数
    unsigned char sysid;              // 定位系统类型
    SAT_INFO sat_info[36];            // 卫星信息
}GSV;
void gsv_parse(GSV *gsv, char *gsv_str);
void gsv_show(GSV *gsv);
#endif

#if ENABLE_VTG
//VTG数据结构体（地面速度信息）
typedef struct {
    double track_true;                // 运动角度，真北参照系
    double track_mag;                 // 运动角度，磁北参照系
    double speed_Kn;                  // 水平运动速度（0.00），节，Knots
    double speed_Km;                  // 水平运动速度（0.00）, 公里/时，km/h
    char   mode;
}VTG;
void vtg_parse(VTG *vtg, char *vtg_str);
void vtg_show(VTG *vtg);
#endif