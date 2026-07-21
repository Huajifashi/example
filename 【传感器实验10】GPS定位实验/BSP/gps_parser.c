#include "gps_parser.h"

GGA gga;
RMC rmc;
GLL gll;
GSA gsa;
GSV gsv;
VTG vtg;

#if ENABLE_GGA
// GGA数据解析
void gga_parse(GGA* gga, char *gga_data){
    char *p = NULL;
    char *next = NULL;
    unsigned char times = 0;

    if(gga == NULL || gga_data == NULL){
        return;
    }

    p = gga_data;
    memset(gga, 0, sizeof(GGA));
    while( p ){
        next = strpbrk(p, ",");
        if(next == p){//字段为空，则跳过
           p = next + 1;
           times++;
           continue;
        }

        if(next == NULL){
            next = strpbrk(p, "*");
            if(next == NULL){break;}
        }
        switch( times ){
            case 1:   // UTC
                next = strpbrk(p, ",");
                memcpy(gga->utc, p, next-p);
                break;
            case 2:   // lat
                gga->lat = strtod(p, NULL);
                break;
            case 3:   // lat dir
                gga->lat_dir = p[0];
                break;
            case 4:   // lon
                gga->lon = strtod(p, NULL);
                break;
            case 5:   // lon dir
                gga->lon_dir = p[0];
                break;
            case 6:   // quality
                gga->quality = (unsigned char)strtol(p, NULL, 10);
                break;
            case 7:   // sats
                gga->sats = (unsigned char)strtol(p, NULL, 10);
                break;
            case 8:   // hdop
                gga->hdop = (unsigned char)strtol(p, NULL, 10);
                break;
            case 9:   // alt
                gga->alt = strtof(p, NULL);
                break;
            case 10:  //M
                break;
            case 11:  // undulation
                gga->undulation = strtof(p, NULL);
                break;
            case 12:  //M
                break;
            case 13:  // age
                gga->age = (unsigned char)strtol(p, NULL, 10);
                break;
            case 14:  // stn_ID
                gga->stn_ID = (unsigned short )strtol(p, NULL, 10);
                break;
            default:
                break;
        }
        p = next + 1;
        times++;
    }
}
void gga_show(GGA *gga){
    printf("================GGA DATA:\r\n");

    printf("utc:%s\r\n", gga->utc);
    printf("lat:%lf\r\n", gga->lat);
    printf("lat_dir:%c\r\n", gga->lat_dir);
    printf("lon:%lf\r\n", gga->lon);
    printf("lon_dir:%c\r\n", gga->lon_dir);
    printf("quality:%u\r\n", gga->quality);
    printf("sats:%u\r\n", gga->sats);
    printf("hdop:%lf\r\n", gga->hdop);
    printf("alt:%lf\r\n", gga->alt);
    printf("undulation:%lf\r\n", gga->undulation);
    printf("age:%u\r\n", gga->age);
    printf("stn_ID:%u\r\n", gga->stn_ID);
}
#endif

#if ENABLE_RMC
// RMC数据解析
void rmc_parse(RMC *rmc, char *rmc_data){
    char *p = NULL;
    char *next = NULL;
    unsigned char times = 0;

    if(rmc == NULL || rmc_data == NULL){
        return;
    }

    p = rmc_data;
    memset(rmc, 0, sizeof(RMC));
    while( p ){
        next = strpbrk(p, ",");

        if(next == p){//字段为空，则跳过
           p = next + 1;
           times++;
           continue;
        }

        if(next == NULL){//已到结尾
            next = strpbrk(p, "*");
            if(next == NULL){break;}
        }

        switch( times ){
            case 1:   // UTC
                memcpy(rmc->utc, p, next-p);
                break;
            case 2:   //status
                rmc->status = p[0];
                break;
            case 3:   // lat
                rmc->lat = strtod(p, NULL);
                break;
            case 4:   // lat dir
                rmc->lat_dir = p[0];
                break;
            case 5:   // lon
                rmc->lon = strtod(p, NULL);
                break;
            case 6:   // lon dir
                rmc->lon_dir = p[0];
                break;
            case 7:   // speen Kn
                rmc->speed_Kn = strtod(p, NULL);
                break;
            case 8:   // track true
                rmc->track_true = strtod(p, NULL);
                break;
            case 9:   //date
                memcpy(rmc->date, p, next-p);
                break;
            case 10:  // mag var
                rmc->mag_var = strtod(p, NULL);
                break;
            case 11:  // mag_var dir
                rmc->mag_var_dir = p[0];
                break;
            case 12:  // mode
                rmc->mode = p[0];
                break;
            case 13:  // nav status
                rmc->nav_status = p[0];
                break;
            default:
                break;
        }
        p = next + 1;
        times++;
    }
}
void rmc_show(RMC *rmc){
    printf("================RMC DATA:\r\n");

    printf("utc:%s\r\n", rmc->utc);
    printf("status:%u\r\n", rmc->status);
    printf("lat:%lf\r\n", rmc->lat);
    printf("lat_dir:%c\r\n", rmc->lat_dir);
    printf("lon:%lf\r\n", rmc->lon);
    printf("lon_dir:%c\r\n", rmc->lon_dir);

    printf("speed_Kn:%lf\r\n", rmc->speed_Kn);
    printf("track_true:%lf\r\n", rmc->track_true);

    printf("date:%s\r\n", rmc->date);
    printf("mag_dir:%lf\r\n", rmc->mag_var);
    printf("mag_var_dir:%u\r\n", rmc->mag_var_dir);
    printf("mode:%c\r\n", rmc->mode);
    printf("nav_status:%c\r\n", rmc->nav_status);

}
#endif

#if ENABLE_GLL
// GLL数据解析
void gll_parse(GLL *gll, char *gll_data){
    char *p = NULL;
    char *next = NULL;
    unsigned char times = 0;

    if(gll ==NULL || gll_data==NULL){
        return;
    }

    p = gll_data;
    memset(gll, 0, sizeof(GLL));
    while (p){
        next = strpbrk(p, ",");
        if(next == p){//字段为空，则跳过
           p = next + 1;
           times++;
           continue;
        }

        if(next == NULL){
            next = strpbrk(p, "*");
            if(next == NULL){break;}
        }
        switch (times){
            case 1:   // lat
                gll->lat = strtod(p, NULL);
                break;
            case 2:   // lat dir
                gll->lat_dir = p[0];
                break;
            case 3:   // lon
                gll->lon = strtod(p, NULL);
                break;
            case 4:   // lon dir
                gll->lon_dir = p[0];
                break;
            case 5:   // utc
                memcpy(gll->utc, p, next-p);
                break;
            case 6:  // status
                gll->status = p[0];
                break;
            default:
                break;
        }
        p = next + 1;
        times++;
    }
}
void gll_show(GLL *gll){
    printf("================GLL DATA:\r\n");

    printf("lat:%lf\r\n", gll->lat);
    printf("lat_dir:%c\r\n", gll->lat_dir);
    printf("lon:%lf\r\n", gll->lon);
    printf("lon_dir:%c\r\n", gll->lon_dir);

    printf("utc:%s\r\n", gll->utc);
    printf("status:%c\r\n", gll->status);
}
#endif

#if ENABLE_GSA
// GSA数据解析
void gsa_parse(GSA *gsa, char *gsa_data){
    char *p = NULL;
    char *next = NULL;
    unsigned char times = 0;

    if(gsa == NULL || gsa_data == NULL){
        return;
    }

    p = gsa_data;
    memset(gsa, 0, sizeof(GSA));
    while( p ){
        next = strpbrk(p, ",");
        if(next == p){//字段为空，则跳过
           p = next + 1;
           times++;
           continue;
        }

        if(next == NULL){
            next = strpbrk(p, "*");
            if(next == NULL){break;}
        }
        switch( times ){
            case 1:   // mode_MA
                gsa->mode_MA = p[0];
                break;
            case 2:   // mode_123
                gsa->mode_123 = p[0];
                break;
            case 15:  // pdop
                gsa->pdop = strtod(p, NULL);
                break; 
            case 16:  // hdop
                gsa->hdop = strtod(p, NULL);
                break;
            case 17:  // vdop
                gsa->vdop = strtod(p, NULL);
            case 18:  //sysid
                gsa->sysid = strtol(p, NULL, 10);
            default:
                break;
        }

        //prn 1-12
        if(times>=3 && times<=14){
            gsa->prn[times-3] = strtol(p, NULL, 10);
        }
        p = next + 1;
        times++;
    }
}
void gsa_show(GSA *gsa){
    int i = 0;
    printf("================GSA DATA:\r\n");

    printf("mode_MA:%u\r\n", gsa->mode_MA);
    printf("mode_123:%u\r\n", gsa->mode_123);

    for(i=0; i<12; i++){
        printf("prn%d:%u\r\n", i, gsa->prn[i]);
    }

    printf("pdop:%lf\r\n", gsa->pdop);
    printf("hdop:%lf\r\n", gsa->hdop);
    printf("vdop:%lf\r\n", gsa->vdop);
    printf("sysid:%u\r\n", gsa->sysid);
}
#endif

#if ENABLE_GSV
// GSV数据解析
void gsv_parse(GSV *gsv, char *gsv_data){
    char *p = NULL;
    char *next = NULL;
    unsigned char times   = 0;
    unsigned char sat_count = 0;//本条消息中卫星计数
    unsigned char sat_index = 0;//整体卫星索引

    if(gsv == NULL || gsv_data == NULL){
        return;
    }

    p = gsv_data;
    while(p){
        next = strpbrk(p, ",");
        if(next == p){//字段为空，则跳过
           p = next + 1;
           times++;
           continue;
        }
        if(next == NULL){
            next = strpbrk(p, "*");
            gsv->sysid = strtol(p, NULL, 10);
            break;
        }

        if(times >= 4){//计算卫星保存位置
            sat_index = (gsv->msg-1)*4 + sat_count;
            if(sat_index >= gsv->sats){
                sat_index = gsv->sats - 1;
            }
        }
        switch( times ){
            case 1:   //msgs
                gsv->msgs = (unsigned char)strtol(p, NULL, 10);
                break;
            case 2:   //msg
                gsv->msg = (unsigned char)strtol(p, NULL, 10);
                break;
            case 3:   //sats
                gsv->sats = (unsigned char)strtol(p, NULL, 10);
                break;
            case 4:   //prn
                gsv->sat_info[sat_index].prn     = strtol(p,NULL,10);
                break;
            case 5:   //elev
                gsv->sat_info[sat_index].elev    = strtol(p,NULL,10);
                break;
            case 6:   //azimuth
                gsv->sat_info[sat_index].azimuth = strtol(p,NULL,10);
                break;
            case 7:   //SNR
                gsv->sat_info[sat_index].SNR     = strtol(p,NULL,10);
                times = 3;
                sat_count++;
                break;
            default:
                break;
        }
        p = next + 1;
        times++;
    }
}
void gsv_show(GSV *gsv){
    int i = 0;
    printf("================GSV DATA:\r\n");

    printf("msgs:%u\r\n", gsv->msgs);
    printf("msg:%u\r\n", gsv->msg);
    printf("sats:%u\r\n", gsv->sats);

    for(i=0; i<gsv->sats; i++){
        printf("-----------sat:%dr\n", i);
        printf("prn:%u\r\n", gsv->sat_info[i].prn);
        printf("elev:%u\r\n", gsv->sat_info[i].elev);
        printf("azimuth:%u\r\n", gsv->sat_info[i].azimuth);
        printf("SNR:%u\r\n", gsv->sat_info[i].SNR);
    }
    printf("sysid:%u\r\n", gsv->sysid);
}
#endif

#if ENABLE_VTG
// VTG数据解析
void vtg_parse(VTG *vtg, char *vtg_data){
    char *p = NULL;
    char *next = NULL;
    unsigned char times = 0;

    if(vtg == NULL || vtg_data == NULL){
        return;
    }

    p = vtg_data;
    memset(vtg, 0, sizeof(VTG));
    while( p ){
        next = strpbrk(p, ",");

        if(next == p){//字段为空，则跳过
           p = next + 1;
           times++;
           continue;
        }

        if(next == NULL){
            next = strpbrk(p, "*");
            if(next == NULL){break;}
        }
        switch (times){
            case 1:   // track true
                vtg->track_true = strtod(p, NULL);
                break;
            case 3:   // track mag
                vtg->track_mag = strtod(p, NULL);
                break;
            case 5:   // speed Kn
                vtg->speed_Kn = strtod(p, NULL);
                break;
            case 7:   // speed Km
                vtg->speed_Km = strtod(p, NULL);
                break;
            case 9:   //mode
                vtg->mode = p[0];
                break;
            default:
                break;
        }
        p = next + 1;
        times++;
    }
}
void vtg_show(VTG *vtg){
    printf("================VTG DATA:\r\n");

    printf("track_true:%lf\r\n", vtg->track_true);
    printf("track_mag:%lf\r\n", vtg->track_mag);
    printf("speed_Kn:%lf\r\n", vtg->speed_Kn);
    printf("speed_Km:%lf\r\n", vtg->speed_Km);
    printf("mode:%c\r\n", vtg->mode);
}
#endif

void gps_parse(char *gps_data){
    if(gps_data == NULL){
        return;
    }

#if ENABLE_GGA
    if(strncmp(gps_data, ID_GGA, strlen(ID_GGA)) == 0){
        gga_parse(&gga, gps_data);
        gga_show( &gga );
        return;
    }
#endif

#if ENABLE_RMC
    if(strncmp(gps_data, ID_RMC, strlen(ID_RMC)) == 0){
        rmc_parse(&rmc, gps_data);
        rmc_show(&rmc );
        return;
    }
#endif

#if ENABLE_GLL
    if(strncmp(gps_data, ID_GLL, strlen(ID_GLL)) == 0){
        gll_parse(&gll, gps_data);
        gll_show( &gll );
        return;
    }
#endif

 #if ENABLE_GSA
    if(strncmp(gps_data, ID_GSA, strlen(ID_GSA)) == 0){
        gsa_parse(&gsa, gps_data);
        gsa_show( &gsa );
        return;
    }
#endif

#if ENABLE_GSV
    if(strncmp(gps_data, ID_GSV, strlen(ID_GSV)) == 0){
        gsv_parse(&gsv, gps_data);
        gsv_show( &gsv );
        return;
    }
#endif

#if ENABLE_VTG
    if(strncmp(gps_data, ID_VTG, strlen(ID_VTG)) == 0){
        vtg_parse(&vtg, gps_data);
        vtg_show( &vtg );
        return;
    }
#endif
}