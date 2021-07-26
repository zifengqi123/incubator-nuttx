#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include "led_ctrl_curl.h"
#include "led_ctrl_pthread.h"
#include "mdnssd_min.h"
#include "led_ctrl_elf.h"


#define _APP_VER_       "V0.0.3"

#define _flog   printf
// #define _flog   _none


// 6.8~11.2v
// FAR char *url_path = "http://192.168.1.102:9988/status.do";
static char url_path[128] = {0};
static char ip[32] = {0};
static int port = 0;

static char http_srv_name[64] = {0};
static char lan_func[64] = {0};
static char net_url[64] = {0};
static char lac_sn[32] = {0};

static bool hasMDNS = false;


int getLocalParam()
{
  cJSON *json = elf_getInitParam();
  if(json != NULL)
  {
    cJSON *jsn = cJSON_GetObjectItem(json, "sn");
    if(jsn != NULL)
    {
      char* csn = cJSON_GetStringValue(jsn);
      if(csn != NULL)
      {
        sprintf(lac_sn, "%s", csn);
      }
    }
    if(json != NULL) cJSON_free(json);
  }


  json = elf_getConfig();
  if(json != NULL)
  {
    cJSON *j1 = cJSON_GetObjectItem(json, "http_srv_name");
    cJSON *j2 = cJSON_GetObjectItem(json, "lan_func");
    cJSON *j3 = cJSON_GetObjectItem(json, "net_url");

    if(j1 != NULL)
    {
      char *c1 = cJSON_GetStringValue(j1);
      if(c1 != NULL)
      {
        sprintf(http_srv_name, "%s", c1);
      }
    }

    if(j2 != NULL)
    {
      char *c2 = cJSON_GetStringValue(j2);
      if(c2 != NULL)
      {
        sprintf(lan_func, "%s", c2);
     }
    }
  
    if(j3 != NULL)
    {
      char *c3 = cJSON_GetStringValue(j3);
      if(c3 != NULL)
      {
        sprintf(net_url, "%s", c3);
      }
    }

    if(json != NULL) cJSON_free(json);

  }

  _flog("sn: %s", lac_sn);
  _flog("http_srv_name: %s\n", http_srv_name);
  _flog("lan_func: %s\n", lan_func);
  _flog("net_url: %s\n", net_url);


}

long getTimestamp()
{
  time_t currtime;
  struct tm now;
  long timestamp;

  if (time(&currtime) == (time_t)-1)
  {
    perror("time");
    return -1;
  }

  return currtime;
}

cJSON *createStatusObject(int i)
{
  cJSON *status;
  struct LED_STATUS *ledstatus = getLedstatusPtr() + (i-1);
  status = cJSON_CreateObject();

  cJSON_AddNumberToObject(status, "index", i);
  if (ledstatus->isOn)
  {
    cJSON_AddStringToObject(status, "status", "ON");
  }
  else
  {
    cJSON_AddStringToObject(status, "status", "OFF");
  }
  cJSON_AddNumberToObject(status, "level", ledstatus->level);
  cJSON_AddNumberToObject(status, "state", ledstatus->state);

  return status;
}

cJSON* packStatus()
{
    cJSON *root;
    cJSON *status;
    char *out;

    int i = 0;

    root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "timestamp", getTimestamp());
    cJSON_AddStringToObject(root, "sn", lac_sn); //todo get sn
    cJSON_AddStringToObject(root, "app_version", _APP_VER_); //todo get sn

    status = cJSON_CreateArray();
    for (i = 1; i <= 8; i++)
    {
      cJSON *t = createStatusObject(i);
      cJSON_AddItemToArray(status, t);
    }

    cJSON_AddItemToObject(root, "led_status", status);

//   out = cJSON_Print(root);
//   _flog("%s\n", out);
//   free(out);
    return root;
}

void parseStatus(cJSON* sts)
{
  int arry_size = cJSON_GetArraySize(sts);
  for (int i = 0; i < arry_size; i++)
  {
    cJSON *ix = cJSON_GetArrayItem(sts, i);

    int index = 0;
    bool ison = false;
    int level = 0;

    cJSON *item = cJSON_GetObjectItem(ix, "index");
    index = item->valueint;
    item = cJSON_GetObjectItem(ix, "status");
    if (strcmp(item->valuestring, "ON") == 0)
    {
      ison = true;
    }

    item = cJSON_GetObjectItem(ix, "level");
    level = item->valueint;

    if (index > 0 && index < 9)
    {
      struct LED_STATUS *newLedStatus = getNewLedStatusPtr() + (index-1);
      struct LED_STATUS *ledstatus = getLedstatusPtr() + (index-1);

      newLedStatus->index = index;
      newLedStatus->isOn = ison;
      newLedStatus->level = level;

      if (newLedStatus->isOn != ledstatus->isOn ||
          newLedStatus->level != ledstatus->level)
      {
        newLedStatus->isChange = true;

        ledstatus->isOn = newLedStatus->isOn;
        ledstatus->level = newLedStatus->level;
        // _flog("Is change: %s %d\n", v2, level);
        _flog("Is change: %d, %d, %d, %d\n", index, ison, level, newLedStatus->isChange);
      }
    }
  }
}

void parseUpdate(cJSON* update)
{
  int arry_size = cJSON_GetArraySize(update);
  int ret = 0;
  bool hasup = false;
  int cnt = 0;

  for (int i = 0; i < arry_size; i++)
  {
    cJSON *ix = cJSON_GetArrayItem(update, i);

    int type = 0;
    char * url = NULL;
    char * ver = NULL;

    cJSON *item = cJSON_GetObjectItem(ix, "type");
    if(item == NULL) return ;

    type = item->valueint;

    item = cJSON_GetObjectItem(ix, "url");
    if(item == NULL) return ;
    url = cJSON_GetStringValue(item);


    item = cJSON_GetObjectItem(ix, "version");
    if(item == NULL) return ;
    ver = cJSON_GetStringValue(item);

    ret = edition_compare(_APP_VER_, ver);

    if(ret < 0) 
    {
      switch (type)
      {
      case 0:
        ret = elf_wget_application(url);
        if(ret >= 0) hasup = true;
        break;
      case 1:
        ret = elf_wget_config(url);
        if(ret >= 0) hasup = true;
        break;
      }
    }
    
  }

  if(hasup)
  {
    _flog("has update..\n");
    ret = elf_writeAppCfgToFlash();
    _flog("elf_writeAppCfgToFlash ret: %d\n", ret);
    if(ret >= 0)
    {
      _flog("++update success.\n");
      elf_sys_reboot();
    }
  }

}

int unPackStatus(cJSON * json)
{
    char *out;

    out = cJSON_Print(json);
    // _flog("%s\n", out);

    cJSON *sts = cJSON_GetObjectItem(json, "led_status");
    if(sts != NULL)
    {
      parseStatus(sts);
    }

    cJSON *update = cJSON_GetObjectItem(json, "update");
    if(update != NULL)
    {
      parseUpdate(update);
    }

    if(out != NULL)
    {
      free(out);
    }

    if(json != NULL)
    {
      cJSON_Delete(json);
      json = NULL;      
    }

    return 0;
}

void pth_mdns_callback()
{

    int ret = getService(http_srv_name, ip, &port);

    _flog("getService ret: %d\n", ret);

    if (ret == 0)
    {
        if (strlen(ip) > 0)
        {
            hasMDNS = true;
            sprintf(url_path, "http://%s:%d/%s", ip, port, lan_func);
            _flog("MDNS IP: %s  Port: %d\n", ip, port);
        }
    }

}

void pth_url_callback()
{
    int ret = 0;
    pthread_t p_mdns;
    int tcnt = 0;

    while (true)
    {
        cJSON *in = packStatus();
        cJSON *out = NULL;
        ret = onLedStatus(url_path, in);

        _flog("-onLedStatus ret: %d\n", ret);
    
        if(ret == -111 || ret == -116) 
        {
            //连接不到服务器
            if(hasMDNS)
            {
                sprintf(url_path, "%s", net_url);
                hasMDNS = false;
                _flog("change url 1: %s\n", url_path);
            }
            else
            {
                ret = pthread_create(&p_mdns, NULL, pth_mdns_callback, NULL);
                if (ret != 0)
                {
                    _flog("thread create p_mdns fail!!!\n");
                }
                pthread_join(p_mdns, NULL);
                _flog("change url 2: %s\n", url_path);

                tcnt ++;

                if(tcnt > 10)
                {
                  elf_sys_reboot();
                }
            }
        }
        else
        {
            tcnt = 0;
        }


        if(in != NULL)
        {
            cJSON_Delete(in);
        }

        if(ret >= 0)
        {
          out = getbackStatus();
          if(out != NULL) {
            unPackStatus(out);
          }
        }
        pth_usleep(200*1000);
    }

}


// void time_test()
// {
//     struct timespec  t1 , t2;

//     t1 = getRealTime();
//     usleep(10 * 1000);
//     t2 = getRealTime();

//     _flog("----usleep(10 * 1000);  ");
//     diffTimespec(t1, t2);

//     t1 = getRealTime();
//     usleep(1000);
//     t2 = getRealTime();
//     _flog("----usleep(1000); ");
//     diffTimespec(t1, t2);


//     t1 = getRealTime();
//     usleep(100);
//     t2 = getRealTime();
//     _flog("----usleep(100); ");
//     diffTimespec(t1, t2);


//     t1 = getRealTime();
//     usleep(10);
//     t2 = getRealTime();
//     _flog("----usleep(10); ");
//     diffTimespec(t1, t2);
// }

int main(int argc, FAR char *argv[])
{
    int ret;
    pthread_t p_url;
    int ndx;
    int status;
    pthread_t p_mdns;

    getLocalParam();

    sprintf(url_path, "%s", net_url);
    
    syslog(LOG_WARNING, "----addon LED CONTROL app started!---\n");
    _flog("url_path: %s\n", url_path);
    // time_test();

    ret = pthread_create(&p_mdns, NULL, pth_mdns_callback, NULL);
    if (ret != 0)
    {
        _flog("thread create p_mdns fail!!!\n");
    }
    pthread_join(p_mdns, NULL);


    ret = pthread_create(&p_url, NULL, pth_url_callback, NULL);
    if (ret != 0)
    {
        _flog("thread create fail!!!\n");
        return ret;
    }

    start_gpio_thread();

    pthread_join(p_url, NULL);


    return 0;
}


