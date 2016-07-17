/*
  IDEAS for commands:
  - INVERTED DAY NIGHT (DARK NIGHT): setweather 0x0006ED5A,1
    give light sources to player (it's really dark)
*/

//NOTE(adm244):
// Seting weather first and than call SetImageSpace with no param
// will result in setting currect imagespace for current weather

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

#include <stdio.h>

#define SCRIPTNAME "SkySilver"
#define CONFIGFILE "skysilver.ini"

#define MAX_SECTION 32767
#define MAX_FILENAME 128
#define MAX_BATCHES 20
#define MAX_STRING 255

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
  BYTE key_skygeddon = IniReadInt(CONFIGFILE, "keys", "iKeySkygeddon", 0x21);
  
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
      //TESObjectCELL * ObjectReference::GetParentCell(TESObjectREFR * self)
      //bool Cell::IsInterior(TESObjectCELL * self)
      
      //TODO(adm244): pointer checks
      
      CActor *player = Game::GetPlayer();
      TESObjectCELL *curCell = ObjectReference::GetParentCell((TESObjectREFR *)player);
      
      if( Cell::IsInterior(curCell) ){
        PrintNote("We're in interior right now, no skygeddon");
      } else{
        
        TESWeather *weather = (TESWeather *)Game::GetFormById(ID_TESWeather::SkyrimMQ206weather);
        SpellItem *spell = (SpellItem *)Game::GetFormById(ID_Spell::dunCGDragonVoiceStormCall);
        
        if( weather && spell ){
          //TODO(adm244): savegame
          //void Cell::RequestSave()
          //void Cell::RequestAutosave()
          
          //void Actor::SetPlayerControls(CActor * self, bool abControls)
          //void Game::ShakeCamera(TESObjectREFR * akSource, float afStrength, float afDuration)
          //bool ObjectReference::Enable(TESObjectREFR * self, bool abFadeIn)
          //void ObjectReference::Say(TESObjectREFR * self, TESTopic * akTopicToSay, CActor * akActorToSpeakAs, bool abSpeakInPlayersHead)
          //uint Sound::Play(TESSound * self, TESObjectREFR * akSource)
          //TESWeather * Weather::GetCurrentWeather()
          //void Weather::ReleaseOverride()
          //bool Actor::AddShout(CActor * self, TESShout * akShout)
          
          //void Cell::SetFogColor(TESObjectCELL * self, uint aiNearRed, uint aiNearGreen, uint aiNearBlue, uint aiFarRed, uint aiFarGreen, uint aiFarBlue)
          //void Cell::SetFogPlanes(TESObjectCELL * self, float afNear, float afFar)
          //void Cell::SetFogPower(TESObjectCELL * self, float afPower)
          
          Weather::ForceActive(weather, 1);
          ExecuteConsoleCommand("setfog 100 5500", NULL);
          
          TESObjectREFR *dragonRef = ObjectReference::PlaceAtMe((TESObjectREFR *)player, Game::GetFormById(0x000FEA9C), 1, 0, 0);
          ObjectReference::MoveTo(dragonRef, (TESObjectREFR *)player, 0, 0, 3000, TRUE);
          //ObjectReference::Enable(dragonRef, TRUE);
          Spell::Cast(spell, dragonRef, NULL);
          
          PrintNote("!skygeddon was successeful");
        }
        
      }
      
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
    }
    
    Wait(0);
  }
}
