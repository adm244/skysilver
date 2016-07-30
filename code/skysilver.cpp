//FIX(adm244): ExecuteConsoleCommand() won't work if console was not initialized

//TODO(adm244): implement a simple log file
//TODO(adm244): implement a cooldown for each command
//TODO(adm244): implement a set chance for each bat file to execute
//TODO(adm244): implement a simple scripting sytem similar to bat files
// with ScriptDragon functionallity exposed to it,
// extends default console functionallity which gives an ability
// to implement user's commands without a need to recompile this plugin
// and any knowlegde of C\C++ programming language (IDEAL)

//IMPORTANT(adm244): skyrim loads MASTER files first and ONLY THEN plugin files,
// so I have to check for how many *.esm files are there in the load order AFTER *.esp
// and apply an offset to plugin index OR just sort the list so all *.esm are higher
//NOTE(adm244): decided not to overcomplicate things and not to worry about this problem

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

//SHGetFolderPathA - for windows xp support
#include <shlobj.h>

#define internal static
#define byte BYTE

#define SCRIPTNAME "SkySilver"
#define PLUGINNAME "SkySilver.esp"
#define DLC02NAME "Dragonborn.esm"
#define CONFIGFILE "skysilver.ini"

#define MAX_SECTION 32767
#define MAX_FILENAME 128
#define MAX_BATCHES 50
#define MAX_STRING 255

enum MsgType{
  MSG_INIT,
  MSG_INFO,
  MSG_ERROR,
  MSG_CMDSTATUS
};

struct BatchData{
  char filename[MAX_FILENAME];
  byte key;
  bool enabled;
};

struct CustomCommand{
  byte key;
  bool enabled;
};

internal BatchData batches[MAX_BATCHES];

internal CustomCommand CommandToggle;
internal CustomCommand CommandSkyeclipse;
internal CustomCommand CommandSkygeddon;

internal bool keys_active = true;
internal bool not_initialized = true;

internal bool show_initmessages;
internal bool show_infomessages;
internal bool show_errormessages;
internal bool show_commandstatus;

internal bool plugins_loaded;
internal bool batches_loaded;

internal int batches_count;

//NOTE(adm244) highest byte will differ depending on load order,
// defaults to 02 (first in load order),
// forced loading update.esm with skysilver plugin,
// so it should be safe
internal uint ID_PLUGIN_WEATHER_ECLIPSE = 0x02000D62;
internal uint ID_PLUGIN_WEATHER_SKYGEDDON = 0x02001828;
internal uint ID_DLC02_SPIDER_FROSTCLOAKING = 0x020206E1;
internal uint ID_DLC02_SPIDER_FIRECLOAKING = 0x02017077;

//NOTE(adm244): displays a message in game if its type is enabled
// 'type' is a type of message
// 'pattern' is a formated message (printf style)
bool ShowMessage(MsgType type, char *pattern, ...)
{
  char text[1024];
  
  if( (!show_initmessages && type == MSG_INIT) ||
      (!show_infomessages && type == MSG_INFO) ||
      (!show_errormessages && type == MSG_ERROR) ||
      (!show_commandstatus && type == MSG_CMDSTATUS) ){
    return(false);
  }
  
  va_list lst;
  va_start(lst, pattern);
  vsprintf_s(text, pattern, lst);
  va_end(lst);
  
  Debug::Notification(text);
  
  return(true);
}

//NOTE(adm244): reads plugins.txt file to determine a load order of plugins used
// returns true if at least one plugin was found
// returns false if none of plugins were found
bool CheckLoadOrder()
{
  bool result_plugin = false;
  bool result_dlc02 = false;
  char path[MAX_STRING];
  FILE *fplugins;
  
  if( SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path) == S_OK ){
    strcat(path, "\\Skyrim\\plugins.txt");
    fplugins = fopen(path, "r");
    
    if( fplugins ){
      char pluginname[MAX_STRING] = PLUGINNAME;
      char dlc02name[MAX_STRING] = DLC02NAME;
      char line[MAX_STRING];
      uint index = 0;
      
      strcat(pluginname, "\n");
      strcat(dlc02name, "\n");
      
      while( !feof(fplugins) ){
        fgets(line, MAX_STRING, fplugins);
        
        if( line[0] != '#' ){
          if( !result_plugin && !strcmp(line, pluginname) ){
            if( index > 0 ){
              uint highbyte = index << 24;
              ID_PLUGIN_WEATHER_ECLIPSE += highbyte;
              ID_PLUGIN_WEATHER_SKYGEDDON += highbyte;
            }
            result_plugin = true;
          }
          if( !result_dlc02 && !strcmp(line, dlc02name) ){
            if( index > 0 ){
              uint highbyte = index << 24;
              ID_DLC02_SPIDER_FROSTCLOAKING += highbyte;
              ID_DLC02_SPIDER_FIRECLOAKING += highbyte;
            }
            result_dlc02 = true;
          }
          
          if( result_plugin && result_dlc02 ){
            break;
          }
          
          index++;
        }
      }
      
      fclose(fplugins);
    }
  }
  
  if( result_plugin ){
    ShowMessage(MSG_INIT, "[INIT] %s was found in load order", PLUGINNAME);
  } else{
    ShowMessage(MSG_ERROR, "[ERROR] %s was not found in load order", PLUGINNAME);
  }
  
  if( result_dlc02 ){
    ShowMessage(MSG_INIT, "[INIT] %s was found in load order", DLC02NAME);
  } else{
    ShowMessage(MSG_ERROR, "[ERROR] %s was not found in load order", DLC02NAME);
  }
  
  return(result_plugin || result_dlc02);
}

//NOTE(adm244): loads a list of batch files and keys that activate them
// filename and associated keycode stored in a BatchData structure pointed to by 'batches'
// number of loaded batch files stored in an integer pointed to by 'num'
bool InitBatchFiles(BatchData *batches, int *num)
{
  char buf[MAX_SECTION];
  char *str = buf;
  int index = 0;
  
  IniReadSection(CONFIGFILE, "batch", buf, MAX_SECTION);
  
  while( true ){
    char *p = strrchr(str, '=');
    
    if( p && (index < MAX_BATCHES) ){
      char *endptr;
      *p++ = '\0';
      
      strcpy(batches[index].filename, str);
      batches[index].key = (int)strtol(p, &endptr, 0);
      batches[index].enabled = true;
      
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

//NOTE(adm244): checks if key was pressed and locks its state
// returns true if key wasn't pressed in previous frame but it is in this one
// returns false if key is not pressed or is hold down
bool IsActivated(byte key, bool *enabled)
{
  if( GetKeyPressed(key) ){
    if( *enabled ){
      *enabled = false;
      return(true);
    }
  } else{
    *enabled = true;
  }
  return(false);
}

//NOTE(adm244): overloaded version for CustomCommand structure
bool IsActivated(CustomCommand *cmd)
{
  return(IsActivated(cmd->key, &cmd->enabled));
}

//NOTE(adm244): initializes plugin variables
// return true if either batch files or plugins were successefully loaded
// return false if all batch files and plugins failed to load
bool init()
{
  show_initmessages = IniReadBool(CONFIGFILE, "settings", "bShowInitMessages", 1);
  show_infomessages = IniReadBool(CONFIGFILE, "settings", "bShowInfoMessages", 1);
  show_errormessages = IniReadBool(CONFIGFILE, "settings", "bShowErrorMessages", 1);
  show_commandstatus = IniReadBool(CONFIGFILE, "settings", "bShowCommandStatus", 1);
  
  keys_active = true;
  plugins_loaded = CheckLoadOrder();
  batches_loaded = InitBatchFiles(batches, &batches_count);
  
  CommandToggle.key = IniReadInt(CONFIGFILE, "keys", "iKeyToggle", 0x24);
  CommandToggle.enabled = true;
  
  CommandSkyeclipse.key = IniReadInt(CONFIGFILE, "keys", "iKeySkyeclipse", 0x2D);
  CommandSkyeclipse.enabled = true;
  
  CommandSkygeddon.key = IniReadInt(CONFIGFILE, "keys", "iKeySkygeddon", 0x21);
  CommandSkygeddon.enabled = true;
  
  ShowMessage(MSG_INFO, "[INFO] %s script launched", SCRIPTNAME);
  
  if( batches_loaded ){
    ShowMessage(MSG_INFO, "[INFO] %d batch files initialized", batches_count);
  } else{
    ShowMessage(MSG_ERROR, "[ERROR] Batch files failed to initialize!");
  }
  
  if( !batches_loaded && !plugins_loaded ){
    ShowMessage(MSG_ERROR, "[FATAL] Couldn't load batches and plugins. Shutting down...");
    return(false);
  }
  
  return(true);
}

//NOTE(adm244): plugin main loop entry point
// ScriptDragon calls us in here
extern "C" void main_loop()
{
  if( not_initialized ){
    if( !init() ){
      return;
    }
    not_initialized = false;
  }
  
  while( true ){
    if( IsActivated(&CommandToggle) ){
      if( keys_active ){
        ShowMessage(MSG_CMDSTATUS, "[STATUS] Commands are disabled");
      } else{
        ShowMessage(MSG_CMDSTATUS, "[STATUS] Commands are enabled");
      }
      keys_active = !keys_active;
    }
    
    if( keys_active ){
      for( int i = 0; i < batches_count; ++i ){
        if( IsActivated(batches[i].key, &batches[i].enabled) ){
          char str[MAX_STRING] = "bat ";
          strcat(str, batches[i].filename);
          
          //FIX(adm244): requires to open (and close) a console first, otherwise will not work
          ExecuteConsoleCommand(str, NULL);
          ShowMessage(MSG_CMDSTATUS, "[STATUS] !%s was successeful", batches[i].filename);
        }
        
        Wait(0);
      }
      
      if( plugins_loaded ){
        if( IsActivated(&CommandSkyeclipse) ){
          CActor *PlayerActor = (CActor *)Game::GetPlayer();
          TESForm *TorchForm = Game::GetFormById(ID_TESObjectLIGH::Torch01);
          
          TESForm *WeatherEclipseForm = Game::GetFormById(ID_PLUGIN_WEATHER_ECLIPSE);
          TESWeather *WeatherEclipse = (TESWeather *)dyn_cast(WeatherEclipseForm, "TESForm", "TESWeather");
          
          if( PlayerActor && TorchForm && WeatherEclipse ){
            Weather::SetActive(WeatherEclipse, true, false);
            
            ObjectReference::AddItem((TESObjectREFR *)PlayerActor, TorchForm, 5, true);
            Actor::EquipItem(PlayerActor, TorchForm, false, true);
            
            Game::RequestAutosave();
            
            ShowMessage(MSG_CMDSTATUS, "[STATUS] !skyeclipse was successeful");
          } else{
            ShowMessage(MSG_ERROR, "[ERROR] Couldn't find \"%s\", check your load order", PLUGINNAME);
          }
        }
      }
      
      if( IsActivated(&CommandSkygeddon) ){
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
            
            //FIX(adm244): get rid of setimagespace, use plugin instead
            ExecuteConsoleCommand("setimagespace 0C10B0", NULL);
            
            //NOTE(adm244): if RandomActor was found then it's an object reference already
            // otherwise it will be player, safe to cast
            ObjectReference::PlaceAtMe((TESObjectREFR *)RandomActor, SpiderFrostCloakingForm, 5, 0, 0);
            ObjectReference::PlaceAtMe((TESObjectREFR *)RandomActor, SpiderFireCloakingForm, 5, 0, 0);
            
            if( SpellBecomeEthereal3 ){
              Spell::Cast(SpellBecomeEthereal3, (TESObjectREFR *)player, NULL);
            }
            
            Game::RequestAutosave();
            
            ShowMessage(MSG_CMDSTATUS, "[STATUS] !skygeddon(dungeon) was successeful");
          } else{
            ShowMessage(MSG_ERROR, "[ERROR] Couldn't find \"%s\", check your load order", DLC02NAME);
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
            ObjectReference::MoveTo(dragon01Ref, (TESObjectREFR *)player, 0, 0, 1500, false);
            
            //NOTE(adm244): dragon01Ref can be safely casted to CActor since it exists
            Actor::StartCombat((CActor *)dragon01Ref, player);
            
            Spell::Cast(spell30sec, dragon01Ref, (TESObjectREFR *)player);
            Spell::Cast(spell90sec, dragon01Ref, NULL);
            
            Weather::ForceActive(weather, 1);
            Game::RequestAutosave();
            
            ShowMessage(MSG_CMDSTATUS, "[STATUS] !skygeddon was successeful");
          } else{
            ShowMessage(MSG_ERROR, "[ERROR] Couldn't find \"%s\", check your load order", PLUGINNAME);
          }
        }
      }
    }
    Wait(0);
  }
}
