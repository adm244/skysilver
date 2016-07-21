//FIX(adm244): ExecuteConsoleCommand() won't work if console was not initialized

//TODO(adm244): implement a set chance for each bat file to execute
//TODO(adm244): implement a cooldown for each command
// perhabs won't be even possible to do, look more into this problem
//TODO(adm244): implement a simple scripting sytem similar to bat files
// with ScriptDragon functionallity exposed to it,
// extends default console functionallity which gives an ability
// to implement user's commands without a need to recompile this plugin
// and any knowlegde of C\C++ programming language (IDEAL)

//IMPORTANT(adm244): skyrim loads MASTER files first and ONLY THEN plugin files,
// so I have to check for how many *.esm files are there in the load order AFTER *.esp
// and apply an offset to plugin index OR just sort the list so all *.esm are higher

//NOTE(adm244): these have no effect at all since fog parameters are specified in weather object
// void Cell::SetFogColor(TESObjectCELL * self, uint aiNearRed, uint aiNearGreen, uint aiNearBlue, uint aiFarRed, uint aiFarGreen, uint aiFarBlue)
// void Cell::SetFogPlanes(TESObjectCELL * self, float afNear, float afFar)
// void Cell::SetFogPower(TESObjectCELL * self, float afPower)

//NOTE(adm244): fog and imagespace will be set to default when entering a map
// or changing cell (going to interior)
// an easy fix is to duplicate the weather and change default imagespace (through an esp plugin)

//NOTE(adm244):
// Setting weather first and than call SetImageSpace with no param
// will result in setting correct imagespace for current weather

//NOTE(adm244):
// The weather will automatically change when crossing cell
// This can be fixed by adding ',1' at the end of the form ID

#include "common\skyscript.h"
#include "common\obscript.h"
#include "common\types.h"
#include "common\enums.h"
#include "common\plugin.h"

//SHGetFolderPathA - for windows 2000 support
#include <shlobj.h>

#define SCRIPTNAME "SkySilver"
#define PLUGINNAME "SkySilver.esp"
#define DLC02NAME "Dragonborn.esm"
#define CONFIGFILE "skysilver.ini"

#define MAX_SECTION 32767
#define MAX_FILENAME 128
#define MAX_BATCHES 20
#define MAX_STRING 255

struct BatchData{
  char filename[MAX_FILENAME];
  int key;
};

//TODO(adm244): move to a plugin_info structure perhabs?
static BOOL not_initialized = TRUE;
static BOOL plugins_loaded = FALSE;

static BatchData batches[MAX_BATCHES];
static int batchnum;

static BOOL keys_active;
static BYTE key_disable;
static BYTE key_skygeddon;
static BYTE key_skyeclipse;

//NOTE(adm244) highest byte will differ depending on load order,
// defaults to 02 (first in load order),
// forced loading update.esm with skysilver plugin,
// so it should be safe
static uint ID_PLUGIN_WEATHER_ECLIPSE = 0x02000D62;
static uint ID_PLUGIN_WEATHER_SKYGEDDON = 0x02001828;
static uint ID_DLC02_SPIDER_FROSTCLOAKING = 0x020206E1;
static uint ID_DLC02_SPIDER_FIRECLOAKING = 0x02017077;

BOOL CheckLoadOrder()
{
  BOOL result_plugin = FALSE;
  BOOL result_dlc02 = FALSE;
  char path[MAX_STRING];
  FILE *plugins;
  
  if( SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path) == S_OK ){
    strcat(path, "\\Skyrim\\plugins.txt");
    plugins = fopen(path, "r");
    
    if( plugins ){
      char pluginname[MAX_STRING] = PLUGINNAME;
      char dlc02name[MAX_STRING] = DLC02NAME;
      char line[MAX_STRING];
      uint index = 0;
      
      strcat(pluginname, "\n");
      strcat(dlc02name, "\n");
      
      while( !feof(plugins) ){
        fgets(line, MAX_STRING, plugins);
        
        if( line[0] != '#' ){
          if( !result_plugin && !strcmp(line, pluginname) ){
            if( index > 0 ){
              uint highbyte = index << 24;
              ID_PLUGIN_WEATHER_ECLIPSE += highbyte;
              ID_PLUGIN_WEATHER_SKYGEDDON += highbyte;
            }
            result_plugin = TRUE;
          }
          if( !result_dlc02 && !strcmp(line, dlc02name) ){
            if( index > 0 ){
              uint highbyte = index << 24;
              ID_DLC02_SPIDER_FROSTCLOAKING += highbyte;
              ID_DLC02_SPIDER_FIRECLOAKING += highbyte;
            }
            result_dlc02 = TRUE;
          }
          
          if( result_plugin && result_dlc02 ){
            break;
          }
          
          index++;
        }
      }
      
      fclose(plugins);
    }
  }
  
  if( !result_plugin ){
    PrintNote("[ERROR] %s was not found in load order", PLUGINNAME);
  }
  if( !result_dlc02 ){
    PrintNote("[ERROR] %s was not found in load order", DLC02NAME);
  }
  
  return(result_plugin || result_dlc02);
}

//NOTE(adm244): loads a list of batch files and keys that activate them
BOOL InitBatchFiles(BatchData *batches, int *num)
{
  char buf[MAX_SECTION];
  char *str = buf;
  int index = 0;
  
  IniReadSection(CONFIGFILE, "batch", buf, MAX_SECTION);
  
  while( TRUE ){
    char *p = strrchr(str, '=');
    
    if( p && (index < MAX_BATCHES) ){
      char *endptr;
      *p++ = '\0';
      
      strcpy(batches[index].filename, str);
      batches[index].key = (int)strtol(p, &endptr, 0);
      
      str = strchr(p, '\0');
      str++;
      
      index++;
    } else{
      break;
    }
    
    Wait(0);
  }
  
  *num = index;
  return(index > 0);
}

void main()
{
  if( not_initialized ){
    BOOL bres = InitBatchFiles(batches, &batchnum);
    
    keys_active = TRUE;
    key_disable = IniReadInt(CONFIGFILE, "keys", "iKeyToggle", 0x24);
    key_skygeddon = IniReadInt(CONFIGFILE, "keys", "iKeySkygeddon", 0x21);
    key_skyeclipse = IniReadInt(CONFIGFILE, "keys", "iKeySkyeclipse", 0x2D);
    
    plugins_loaded = CheckLoadOrder();
    
    PrintNote("[INFO] %s script launched", SCRIPTNAME);
    
    if( bres ){
      PrintNote("[INFO] %d batch files initialized", batchnum);
    } else{
      PrintNote("[ERROR] Batch files failed to initialize!");
      return;
    }
    
    not_initialized = FALSE;
  }
  
  while( TRUE ){
    if( GetKeyPressed(key_disable) ){
      if( keys_active ){
        PrintNote("[INFO] Commands disabled");
      } else{
        PrintNote("[INFO] Commands enabled");
      }
      keys_active = !keys_active;
      
      Wait(500);
    }
    
    if( keys_active ){
      for( int i = 0; i < batchnum; ++i ){
        if( GetKeyPressed(batches[i].key) ){
          char str[MAX_STRING] = "bat ";
          strcat(str, batches[i].filename);
          
          //FIX(adm244): requires to open (and close) a console first, otherwise will not work
          ExecuteConsoleCommand(str, NULL);
          PrintNote("!%s was successeful", batches[i].filename);
        }
        
        Wait(0);
      }
      
      if( plugins_loaded ){
        if( GetKeyPressed(key_skyeclipse) ){
          CActor *PlayerActor = (CActor *)Game::GetPlayer();
          TESForm *TorchForm = Game::GetFormById(ID_TESObjectLIGH::Torch01);
          
          TESForm *WeatherEclipseForm = Game::GetFormById(ID_PLUGIN_WEATHER_ECLIPSE);
          TESWeather *WeatherEclipse = (TESWeather *)dyn_cast(WeatherEclipseForm, "TESForm", "TESWeather");
          
          if( PlayerActor && TorchForm && WeatherEclipse ){
            Weather::SetActive(WeatherEclipse, TRUE, FALSE);
            
            ObjectReference::AddItem((TESObjectREFR *)PlayerActor, TorchForm, 5, TRUE);
            Actor::EquipItem(PlayerActor, TorchForm, FALSE, TRUE);
            
            Game::RequestAutosave();
            
            PrintNote("!skyeclipse was successeful");
          } else{
            PrintNote("[ERROR] Couldn't find \"%s\", check your load order", PLUGINNAME);
          }
          
          Wait(500);
        }
        
        if( GetKeyPressed(key_skygeddon) ){
          CActor *player = Game::GetPlayer();
          TESObjectCELL *curCell = ObjectReference::GetParentCell((TESObjectREFR *)player);
          
          if( Cell::IsInterior(curCell) ){
            // INTERIOR
            TESForm *SpiderFrostCloakingForm = (TESForm *)dyn_cast(Game::GetFormById(ID_DLC02_SPIDER_FROSTCLOAKING), "TESForm", "TESForm");
            TESForm *SpiderFireCloakingForm = (TESForm *)dyn_cast(Game::GetFormById(ID_DLC02_SPIDER_FIRECLOAKING), "TESForm", "TESForm");
            
            SpellItem *SpellBecomeEthereal3 = (SpellItem *)Game::GetFormById(ID_SpellItem::VoiceBecomeEthereal3);
            
            if( SpiderFrostCloakingForm && SpiderFireCloakingForm ){
              CActor *RandomActor;
              
              float x = ObjectReference::GetPositionX((TESObjectREFR *)player);
              float y = ObjectReference::GetPositionY((TESObjectREFR *)player);
              float z = ObjectReference::GetPositionZ((TESObjectREFR *)player);
              
              RandomActor = Game::FindRandomActor(x, y, z, 3000.0);
              
              ExecuteConsoleCommand("setimagespace 0C10B0", NULL);
              
              //NOTE(adm244): if RandomActor was found then it's an object reference already
              // otherwise it will be player, safe to cast
              ObjectReference::PlaceAtMe((TESObjectREFR *)RandomActor, SpiderFrostCloakingForm, 5, 0, 0);
              ObjectReference::PlaceAtMe((TESObjectREFR *)RandomActor, SpiderFireCloakingForm, 5, 0, 0);
              
              if( SpellBecomeEthereal3 ){
                Spell::Cast(SpellBecomeEthereal3, (TESObjectREFR *)player, NULL);
              }
              
              Game::RequestAutosave();
              
              PrintNote("!skygeddon(dungeon) was successeful");
            } else{
              PrintNote("[ERROR] Couldn't find \"%s\", check your load order", DLC02NAME);
            }
          } else{
            // EXTERIOR
            TESWeather *weather = (TESWeather *)Game::GetFormById(ID_PLUGIN_WEATHER_SKYGEDDON);
            SpellItem *spell30sec = (SpellItem *)Game::GetFormById(ID_Spell::DragonVoiceStormCall);
            SpellItem *spell90sec = (SpellItem *)Game::GetFormById(ID_Spell::dunCGDragonVoiceStormCall);
            
            if( weather && spell30sec && spell90sec ){
              TESForm *dragonForm = Game::GetFormById(ID_TESLevCharacter::MQ104LCharDragon);
              TESObjectREFR *dragon01Ref = ObjectReference::PlaceAtMe((TESObjectREFR *)player, dragonForm, 1, 0, 0);
              
              Actor::ResetHealthAndLimbs(player);
              ObjectReference::MoveTo(dragon01Ref, (TESObjectREFR *)player, 0, 0, 1500, FALSE);
              
              //NOTE(adm244): dragon01Ref can be safely casted to CActor since it exists
              Actor::StartCombat((CActor *)dragon01Ref, player);
              
              Spell::Cast(spell30sec, dragon01Ref, (TESObjectREFR *)player);
              Spell::Cast(spell90sec, dragon01Ref, NULL);
              
              Weather::ForceActive(weather, 1);
              Game::RequestAutosave();
              
              PrintNote("!skygeddon was successeful");
            } else{
              PrintNote("[ERROR] Couldn't find \"%s\", check your load order", PLUGINNAME);
            }
            
          }
          
          Wait(500);
        }
      }
    }
    
    Wait(0);
  }
}
