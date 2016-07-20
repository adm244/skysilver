/*
  IDEAS for commands:
  - INVERTED DAY NIGHT (DARK NIGHT): setweather 0x0006ED5A,1
    give light sources to player (it's really dark)
*/

//void Game::RequestSave()
//void Game::RequestAutosave()

//void Actor::SetPlayerControls(CActor * self, bool abControls)
//void Game::ShakeCamera(TESObjectREFR * akSource, float afStrength, float afDuration)
//bool ObjectReference::Enable(TESObjectREFR * self, bool abFadeIn)
//void ObjectReference::Say(TESObjectREFR * self, TESTopic * akTopicToSay, CActor * akActorToSpeakAs, bool abSpeakInPlayersHead)
//uint Sound::Play(TESSound * self, TESObjectREFR * akSource)
//TESWeather * Weather::GetCurrentWeather()
//void Weather::ReleaseOverride()
//bool Actor::AddShout(CActor * self, TESShout * akShout)

//NOTE(adm244): these have no affect at all...
// void Cell::SetFogColor(TESObjectCELL * self, uint aiNearRed, uint aiNearGreen, uint aiNearBlue, uint aiFarRed, uint aiFarGreen, uint aiFarBlue)
// void Cell::SetFogPlanes(TESObjectCELL * self, float afNear, float afFar)
// void Cell::SetFogPower(TESObjectCELL * self, float afPower)

//void Actor::StartCombat(CActor * self, CActor * akTarget)

//ISBoatedMansGrottoFog = 0x00105939 - orange
//ISSkyrimClearDUSK_CO = 0x0010A1D1 - orange
//ISSkyrimFogDAWN_CO = 0x0010A1D3 - gray
//ISSkyrimFogDUSK_FF = 0x0010A1F9 - orange, dof
//ISSkyrimOvercastWarDAWN = 0x000D299B, orange
//ISSolitudeBluePalaceFogNMARE = 0x0010593C, orange, close dof

//NOTE(adm244): fog and imagespace will be set to default when entering a map
// or changing cell (going to interior)
// perhabs track going into interior?

//NOTE(adm244):
// Setting weather first and than call SetImageSpace with no param
// will result in setting correct imagespace for current weather

//NOTE(adm244):
// The weather will automatically change when crossing cell
// This can be fixed by adding ',1' at the end of the form ID

// CONSOLE: ForceAction ID_Action
// CONSOLE: SetImageSpace ID_ImageSpace
// CONSOLE: SetWeather ID_Weather
// CONSOLE: ForceWeather ID_Weather

// CONSOLE: TeachWord ID_Word
// AlduinFirestormShout words: 000252C3, 000252C4, 000252C5

// CONSOLE: setfog START END (setfog 100 5500 - foggy, setfog 0 0 - applies default fog)
// CONSOLE: setclipdist DIST

//CActor * Game::GetPlayer()
//TESForm * Game::GetFormById(uint aiFormID)
//bool Actor::AddShout(CActor * self, TESShout * akShout)
//void Actor::EquipShout(CActor * self, TESShout * akShout)
//void Weather::ForceActive(TESWeather * self, bool abOverride)
//SkyrimMQ206weather = 0x0010199F

//bool Spell::Cast(SpellItem * self, TESObjectREFR * akSource, TESObjectREFR * akTarget)
//DragonVoiceStormCall = 0x00016D40
//dunCGDragonVoiceStormCall = 0x000B2388
  
//bool ObjectReference::MoveTo(TESObjectREFR * self, TESObjectREFR * akTarget, float afXOffset, float afYOffset, float afZOffset, bool abMatchRotation)
//lvlMQDragon = 0x000FEA9A
//MQLCharDragonFire = 0x000FEA9C

#include "common\skyscript.h"
#include "common\obscript.h"
#include "common\types.h"
#include "common\enums.h"
#include "common\plugin.h"

//SHGetFolderPathA - for windows 2000 support
#include <shlobj.h>

#define SCRIPTNAME "SkySilver"
#define PLUGINNAME "SkySilver.esp"
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
static BOOL initialized = FALSE;

static BatchData batches[MAX_BATCHES];
static int batchnum;

static BOOL keys_active;
static BYTE key_disable;
static BYTE key_skygeddon;
static BYTE key_skyeclipse;

//NOTE(adm244) highest byte will differ depending on load order,
// defaults to 02 (first in load order)
static uint ID_PLUGIN_WEATHER_ECLIPSE = 0x02000D62;
static uint ID_PLUGIN_WEATHER_SKYGEDDON = 0x02001828;

BOOL CheckLoadOrder()
{
  BOOL result = FALSE;
  char path[MAX_STRING];
  FILE *plugins;
  
  if( SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path) == S_OK ){
    strcat(path, "\\Skyrim\\plugins.txt");
    plugins = fopen(path, "r");
    
    if( plugins ){
      char filename[MAX_STRING] = PLUGINNAME;
      char line[MAX_STRING];
      uint index = 0;
      
      strcat(filename, "\n");
      
      while( !feof(plugins) ){
        fgets(line, MAX_STRING, plugins);
        if( line[0] != '#' ){
          if( !strcmp(line, filename) ){
            if( index > 0 ){
              index <<= 24;
              ID_PLUGIN_WEATHER_ECLIPSE += index;
              ID_PLUGIN_WEATHER_SKYGEDDON += index;
            }
            
            result = TRUE;
            break;
          }
          index++;
        }
      }
      
      fclose(plugins);
    }
  }
  
  return(result);
}

//NOTE(adm244): loads a list of batch files and keys that activate them
BOOL InitBatchFiles(BatchData *batches, int *num)
{
  char buf[MAX_SECTION];
  char *str = buf;
  int batchnum = 0;
  
  IniReadSection(CONFIGFILE, "batch", buf, MAX_SECTION);
  
  while( TRUE ){
    char *p = strrchr(str, '=');
    
    if( p && (batchnum < MAX_BATCHES) ){
      char *endptr;
      *p++ = '\0';
      
      strcpy(batches[batchnum].filename, str);
      batches[batchnum].key = (int)strtol(p, &endptr, 0);
      
      str = strchr(p, '\0');
      str++;
      
      batchnum++;
    } else{
      break;
    }
    
    Wait(0);
  }
  
  *num = batchnum;
  return(batchnum > 0);
}

void main()
{
  if( !initialized ){
    BOOL bres = InitBatchFiles(batches, &batchnum);
    
    keys_active = TRUE;
    key_disable = IniReadInt(CONFIGFILE, "keys", "iKeyToggle", 0x24);
    key_skygeddon = IniReadInt(CONFIGFILE, "keys", "iKeySkygeddon", 0x21);
    key_skyeclipse = IniReadInt(CONFIGFILE, "keys", "iKeySkynight", 0x2D);
    
    if( !CheckLoadOrder() ){
      PrintNote("[FATAL] Could not find \"%s\" plugin in active load order", PLUGINNAME);
      return;
    }
    
    PrintNote("[INFO] %s script launched", SCRIPTNAME);
    
    if( bres ){
      PrintNote("[INFO] %d batch files initialized", batchnum);
    } else{
      PrintNote("[ERROR] Batch files failed to initialize!");
      return;
    }
    
    initialized = TRUE;
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
    
    if( keys_active ){
      if( GetKeyPressed(key_skyeclipse) ){
        CActor *PlayerActor = (CActor *)Game::GetPlayer();
        TESForm *TorchForm = Game::GetFormById(ID_TESObjectLIGH::Torch01);
        
        TESForm *WeatherEclipseForm = Game::GetFormById(ID_PLUGIN_WEATHER_ECLIPSE);
        TESWeather *WeatherEclipse = (TESWeather *)dyn_cast(WeatherEclipseForm, "TESForm", "TESWeather");
        
        if( PlayerActor && TorchForm && WeatherEclipse ){
          Weather::SetActive(WeatherEclipse, TRUE, FALSE);
          
          ObjectReference::AddItem((TESObjectREFR *)PlayerActor, TorchForm, 5, TRUE);
          Actor::EquipItem(PlayerActor, TorchForm, FALSE, TRUE);
          
          Wait(10);
          
          Game::RequestAutosave();
          
          PrintNote("!skyeclipse was successeful");
        } else{
          PrintNote("[ERROR] Weather %08X not found, check your load order", ID_PLUGIN_WEATHER_ECLIPSE);
        }
        
        Wait(500);
      }
      
      if( GetKeyPressed(key_skygeddon) ){
        //TODO(adm244): pointer checks
        
        CActor *player = Game::GetPlayer();
        TESObjectCELL *curCell = ObjectReference::GetParentCell((TESObjectREFR *)player);
        
        if( Cell::IsInterior(curCell) ){
          // INTERIOR
          TESForm *SpiderFrostCloakingForm = (TESForm *)dyn_cast(Game::GetFormById(0x030206E1), "TESForm", "TESForm");
          TESForm *SpiderFireCloakingForm = (TESForm *)dyn_cast(Game::GetFormById(0x03017077), "TESForm", "TESForm");
          
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
          }
        } else{
          // EXTERIOR
          TESWeather *weather = (TESWeather *)Game::GetFormById(ID_PLUGIN_WEATHER_SKYGEDDON);
          SpellItem *spell30sec = (SpellItem *)Game::GetFormById(ID_Spell::DragonVoiceStormCall);
          SpellItem *spell90sec = (SpellItem *)Game::GetFormById(ID_Spell::dunCGDragonVoiceStormCall);
          
          if( weather && spell30sec && spell90sec ){
            TESForm *dragonForm = Game::GetFormById(0x000F8A4D);
            TESObjectREFR *dragon01Ref = ObjectReference::PlaceAtMe((TESObjectREFR *)player, dragonForm, 1, 0, 0);
            
            Actor::ResetHealthAndLimbs(player);
            ObjectReference::MoveTo(dragon01Ref, (TESObjectREFR *)player, 0, 0, 1500, FALSE);
            
            //NOTE(adm244): dragon01Ref can be safely casted to CActor since it exists
            Actor::StartCombat((CActor *)dragon01Ref, player);
            
            Spell::Cast(spell30sec, dragon01Ref, (TESObjectREFR *)player);
            Spell::Cast(spell90sec, dragon01Ref, NULL);
            
            Weather::ForceActive(weather, 1);
            
            Wait(10);
            
            Game::RequestAutosave();
            
            PrintNote("!skygeddon was successeful");
          }
          
        }
        
        Wait(500);
      }
      
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
    }
    
    Wait(0);
  }
}
