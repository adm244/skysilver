#include "common\skyscript.h"
#include "common\obscript.h"
#include "common\types.h"
#include "common\enums.h"
#include "common\plugin.h"

#include <stdio.h>

#define SCRIPTNAME "SkySilver"
#define CONFIGFILE "skysilver.ini"

#define MAX_SECTION 32767
#define MAX_FILENAME 128
#define MAX_BATCHES 20
#define MAX_COMMAND MAX_FILENAME+4

struct BatchData{
  char filename[MAX_FILENAME];
  int key;
};

//NOTE(adm244): loads a list of batch files and keys that activate them
bool InitBatchFiles(BatchData *batches, int *num)
{
  char buf[MAX_SECTION];
  char *str = buf;
  int batchnum = 0;
  
  IniReadSection(CONFIGFILE, "batch", buf, MAX_SECTION);
  
  while( TRUE ){
    char *p = strrchr(str, '=');
    
    if( p && (batchnum < MAX_BATCHES) ){
      char *endptr;
      //std::string key;
      
      *p++ = '\0';
      
      strcpy(batches[batchnum].filename, str);
      batches[batchnum].key = (int)strtol(p, &endptr, 0);
      
      //key = GetKeyName(batches[batchnum].key);
      //PrintNote("[INFO] %s file, %s key loaded", batches[batchnum].filename, key.c_str());
      
      str = strchr(p, '\0');
      str++;
      
      batchnum++;
    } else{
      break;
    }
    
    Wait(0);
  }
  
  *num = batchnum;
  return( batchnum > 0 );
}

void main()
{
  BatchData batches[MAX_BATCHES];
  int batchnum;
  
  BOOL bres = InitBatchFiles(batches, &batchnum);
  
  BOOL keys_active = TRUE;
  BYTE key_disable = IniReadInt(CONFIGFILE, "keys", "iKeyToggle", 0x24);
  BYTE key_skygeddon = IniReadInt(CONFIGFILE, "keys", "iKeySkygeddon", VK_PAGEUP);
  
  PrintNote("[INFO] %s script launched", SCRIPTNAME);
  
  if( bres ){
    PrintNote("[INFO] %d batch files initialized", batchnum);
  } else{
    PrintNote("[ERROR] Batch files failed to initialize!");
    return;
  }
  
  while( TRUE )
  {
    if( GetKeyPressed(key_disable) ){
      if( keys_active ){
        PrintNote("[INFO] Commands disabled");
      } else{
        PrintNote("[INFO] Commands enabled");
      }
      keys_active = !keys_active;
      
      Wait(500);
    }
    
    if( GetKeyPressed(key_skygeddon) ){
      //NOTE(adm244):
      // Seting weather first and than call SetImageSpace with no param
      // will result in setting currect imagespace for current weather
      
      //NOTE(adm244):
      // The weather will automatically change when crossing cell
      
      // CONSOLE: ForceAction ID_Action
      // CONSOLE: SetImageSpace ID_ImageSpace
      // CONSOLE: SetWeather ID_Weather
      // CONSOLE: ForceWeather ID_Weather
      
      //CActor * GetPlayer()
      //TESForm * GetFormById(uint aiFormID)
      //bool AddShout(CActor * self, TESShout * akShout)
      //void EquipShout(CActor * self, TESShout * akShout)
      
      CActor *player = Game::GetPlayer();
      TESShout *stormcall = (TESShout *)Game::GetFormById(ID_TESShout::AlduinFirestormShout);
      
      if( player && stormcall ){
        Actor::AddShout(player, stormcall);
        Actor::EquipShout(player, stormcall);
        
        PrintNote("[INFO] Dragon Storm Call ready to use");
      }
      
      Wait(500);
    }
    
    if( keys_active ){
      for( int i = 0; i < batchnum; ++i ){
        if( GetKeyPressed(batches[i].key) ){
          char str[MAX_COMMAND] = "!";
          strcat(str, batches[i].filename);
          strcat(str, " was successefull");
          
          PrintNote(str);
          
          strcpy(str, "bat ");
          strcat(str, batches[i].filename);
          
          //FIX(adm244): requires to open (and close) a console first, otherwise will not work
          ExecuteConsoleCommand(str, NULL);
        }
        
        Wait(0);
      }
    }
    
    Wait(0);
  }
}

