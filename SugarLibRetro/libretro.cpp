#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#define NO_MULTITHREAD 
#define NOFILTER 
#define NOZLIB 
#define NO_RAW_FORMAT

#include "Machine.h"

#include "libretro.h"

//#define WIDTH  768
#define WIDTH  640
//#define HEIGHT 277
#define HEIGHT 480

//#define OFFSET_X 143
#define OFFSET_X 207
//#define OFFSET_Y 47
#define OFFSET_Y 84


#define M_PI    3.14159265358979323846264338327950288   /* pi */

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

// Display
class RetroDisplay : public IDisplay
{
public:
   RetroDisplay()
   {
      // Init
      video_buffer = new int[1024 * 1024];
      memset(video_buffer, 0, 1024 * 1024 * sizeof(int));
      pitch_ = 1024 * sizeof(unsigned int);
   };
   virtual ~RetroDisplay() {};

   virtual void SetScanlines(int scan) {};
   virtual void Display() {};
   virtual bool AFrameIsReady() { return true; };
   virtual void Config() {};
   virtual const char* GetInformations() { return "Libretro GDI"; };
   virtual int GetWidth() { return WIDTH; };
   virtual int GetHeight() { return HEIGHT; };
   virtual void VSync(bool bDbg)
   {
      video_cb(&video_buffer[OFFSET_X  + 1024*OFFSET_Y], WIDTH, HEIGHT, pitch_);
   }
   virtual void StartSync(){};
   virtual void WaitVbl() {};
   virtual int* GetVideoBuffer(int y)
   {
      return &video_buffer[1024 * y*2];
   }
   virtual void Reset() {};
   virtual void Screenshot() {};
   virtual void ScreenshotEveryFrame(int bSetOn) {};
   virtual bool IsEveryFrameScreened() {
      return false;
   }
   virtual bool IsDisplayed() { return true;/* m_bShow;*/ };
   virtual void FullScreenToggle() {};
   virtual void ForceFullScreen(bool bSetFullScreen) {}
   virtual void WindowChanged(int xIn, int yIn, int wndWidth, int wndHeight) {};
   virtual bool SetSyncWithVbl(int speed) { return false; };
   virtual bool IsWaitHandled() { return false; };
   virtual bool GetBlackScreenInterval() { return false; };
   virtual void SetBlackScreenInterval(bool bBS) { };

   void Init();
   void Show(bool bShow);

   virtual void SetSize(SizeEnum size) {};
   virtual SizeEnum  GetSize() { return S_STANDARD; };

   virtual void ResetLoadingMedia() {};
   virtual void SetLoadingMedia() {};

   virtual void ResetDragnDropDisplay() {};
   virtual void SetDragnDropDisplay(int type) {};
   virtual void SetCurrentPart(int x, int y) {};
   virtual int GetDnDPart() { return 0; };

protected:
   int * video_buffer;
   unsigned int pitch_;
};

class Keyboard : public IKeyboardHandler
{
public:
   Keyboard() :
      gamepad_button_left_(false),
      gamepad_button_right_(false),
      gamepad_button_top_(false),
      gamepad_button_bottom_(false),
      gamepad_button_X_(false),
      gamepad_button_A_(false)
   {}
   virtual ~Keyboard() {}

   virtual unsigned char GetKeyboardMap(int index)
   {
/*      if (index == 5)
      {
         unsigned char result = 0xFF;
         // button 1
         if (gamepad_state_.buttons & GamePadButtonStart)result &= ~0x80;
         return result;
      }*/
      if (index == 9)
      {
         unsigned char result = 0xFF;
         // button 1
         if (gamepad_button_X_) result &= ~0x10;
         // button 2
         if (gamepad_button_A_) result &= ~0x20;
         // buttons up
         if (gamepad_button_top_) result &= ~0x1;
         // buttons down
         if (gamepad_button_bottom_) result &= ~0x2;
         // buttons left
         //if (gamepad_state_.buttons & GamePadButtonLeft) result &= ~0x4;
         if ( gamepad_button_left_ ) result &= ~0x4;
         // buttons right
         if (gamepad_button_right_) result &= ~0x8;
         
         return result;
      }

      return 0xFF;
   }
   virtual void Init(bool* register_replaced)
   {

   }
   virtual void ForceKeyboardState(unsigned char key_states[10])
   {

   }
   bool gamepad_button_left_;
   bool gamepad_button_right_;
   bool gamepad_button_top_;
   bool gamepad_button_bottom_;
   bool gamepad_button_X_;
   bool gamepad_button_A_;
};
class ConfigurationManager : public IConfiguration
{
public:
   virtual void SetConfiguration(const char* section, const char* cle, const char* valeur, const char* file)
   {
      struct retro_variable var;// = { var.key = "test_aspect" };
      std::string key;
      key = section + std::string("_") + std::string(cle);
      var.key = key.c_str();
      var.value = valeur;
      environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, &var);
   }
   virtual unsigned int GetConfiguration(const char* section, const char* cle, const char* default_value, char* out_buffer, unsigned int buffer_size, const char* file)
   {
      if (section == nullptr ||cle == nullptr)
      {
         strncpy(out_buffer, default_value, buffer_size);
         return true;
      }
      struct retro_variable var;// = { var.key = "test_aspect" };
      std::string key;
      key = section + std::string("_") + std::string(cle);
      var.key = key.c_str();
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      {
         strncpy(out_buffer, var.value, buffer_size);
         return true;
      }
      else
      {
         strncpy(out_buffer, default_value, buffer_size);
         return true;
      }      
   }
   virtual unsigned int GetConfigurationInt(const char* section, const char* cle, unsigned int default_value, const char* file)
   {
      struct retro_variable var;// = { var.key = "test_aspect" };
      std::string key;
      key = section + std::string("_") + std::string(cle);
      var.key = key.c_str();
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      {
         return atoi(var.value);
         
      }
      else
      {
         return default_value;
      }
   }
};



static struct retro_log_callback logging;
static retro_log_printf_t log_cb;
static bool use_audio_cb;
static float last_aspect;
static float last_sample_rate;

// Definition of emulator
static EmulatorEngine* emulator_ = nullptr;
static Motherboard * motherboard_ = nullptr;
static ConfigurationManager conf_manager_;
static RetroDisplay display_;
static Keyboard keyboard_;

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
   (void)level;
   va_list va;
   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
}

int LoadCprFromBuffer(unsigned char* buffer, int size)
{
   // Check RIFF chunk
   int index = 0;
   if (size >= 12
      && (memcmp(&buffer[0], "RIFF", 4) == 0)
      && (memcmp(&buffer[8], "AMS!", 4) == 0)
      )
   {
      // Reinit Cartridge
      motherboard_->EjectCartridge();

      // Ok, it's correct.
      index += 4;
      // Check the whole size

      int chunk_size = buffer[index]
         + (buffer[index + 1] << 8)
         + (buffer[index + 2] << 16)
         + (buffer[index + 3] << 24);

      index += 8;

      // 'fmt ' chunk ? skip it
      if (index + 8 < size && (memcmp(&buffer[index], "fmt ", 4) == 0))
      {
         index += 8;
      }

      // Good.
      // Now we are at the first cbxx
      while (index + 8 < size)
      {
         if (buffer[index] == 'c' && buffer[index + 1] == 'b')
         {
            index += 2;
            char buffer_block_number[3] = { 0 };
            memcpy(buffer_block_number, &buffer[index], 2);
            int block_number = (buffer_block_number[0] - '0') * 10 + (buffer_block_number[1] - '0');
            index += 2;

            // Read size
            int block_size = buffer[index]
               + (buffer[index + 1] << 8)
               + (buffer[index + 2] << 16)
               + (buffer[index + 3] << 24);
            index += 4;

            if (block_size <= size && block_number < 256)
            {
               // Copy datas to proper ROM
               unsigned char* rom = motherboard_->GetCartridge(block_number);
               memset(rom, 0, 0x1000);
               memcpy(rom, &buffer[index], block_size);
               index += block_size;
            }
            else
            {
               return -1;
            }
         }
         else
         {
            return -1;
         }
      }
   }
   else
   {
      // Incorrect headers
      return -1;
   }

   return 0;
}

void retro_init(void)
{
   motherboard_ = new Motherboard(nullptr, &keyboard_);

   // Create 
   motherboard_->SetPlus(true);
   motherboard_->InitMotherbard(nullptr, nullptr, &display_, nullptr, nullptr, nullptr);
   motherboard_->GetPSG()->InitSound(nullptr);

   motherboard_->OnOff();
   motherboard_->GetMem()->InitMemory();
   motherboard_->GetMem()->SetRam(1);
   motherboard_->GetCRTC()->DefinirTypeCRTC(CRTC::AMS40226);
   motherboard_->GetVGA()->SetPAL(true);

   // load CPR
   /*
#define CPR_FILE "Barbarian - The Ultimate Warrior.cpr"
   FILE* f;
   unsigned char* buffer_ = nullptr;
   f = fopen( CPR_FILE, "rb");
   if ( f != nullptr)
   {
      fseek(f, 0, SEEK_END);
      unsigned int buffer_size_ = ftell(f);
      rewind(f);
      unsigned char* buffer_ = new unsigned char[buffer_size_];

      fread(buffer_, buffer_size_, 1, f);
      LoadCprFromBuffer(buffer_, buffer_size_);
      fclose(f);
   }
   */
   motherboard_->GetPSG()->Reset();
   motherboard_->GetSig()->Reset();
   motherboard_->InitStartOptimizedPlus();
   motherboard_->OnOff();

}

void retro_deinit(void)
{
   delete emulator_;
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   log_cb(RETRO_LOG_INFO, "Plugging device %u into port %u.\n", device, port);
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name = "Sugarbox";
   info->library_version = "v1.00";
   info->need_fullpath = true;
   info->valid_extensions = "cpr"; // Anything is fine, we don't care.
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   float aspect = 4.0f / 3.0f;
   struct retro_variable var = { var.key = "test_aspect" };
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "4:3"))
         aspect = 4.0f / 3.0f;
      else if (!strcmp(var.value, "16:9"))
         aspect = 16.0f / 9.0f;
   }

   float sampling_rate = 44100.0f;
   var.key = "test_samplerate";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      sampling_rate = strtof(var.value, NULL);

   //info->timing = (struct retro_system_timing);
   info->timing.fps = 50.0;
   info->timing.sample_rate = sampling_rate;
   
   //info->geometry = (struct retro_game_geometry) {
   info->geometry.base_width = WIDTH;
   info->geometry.base_height = HEIGHT;

   info->geometry.max_width = WIDTH;
   info->geometry.max_height = HEIGHT;
   info->geometry.aspect_ratio = aspect;

   last_aspect = aspect;
   last_sample_rate = sampling_rate;
}

static struct retro_rumble_interface rumble;

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   static const struct retro_variable vars[] = {
      { "test_aspect", "Aspect Ratio; 4:3|16:9" },
      { "test_samplerate", "Sample Rate; 30000|20000" },
      { "test_opt0", "Test option #0; false|true" },
      { "test_opt1", "Test option #1; 0" },
      { "test_opt2", "Test option #2; 0|1|foo|3" },
      { NULL, NULL },
   };

   cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void*)vars);

   bool no_content = false;
   cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

   if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
      log_cb = logging.log;
   else
      log_cb = fallback_log;

   static const struct retro_controller_description controllers[] = {
      { "Dummy Controller #1", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0) },
      { "Dummy Controller #2", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 1) },
      { "Augmented Joypad", RETRO_DEVICE_JOYPAD }, // Test overriding generic description in UI.
   };

   static const struct retro_controller_info ports[] = {
      { controllers, 3 },
      { NULL, 0 },
   };

   cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);



}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

static unsigned x_coord;
static unsigned y_coord;
static unsigned phase;
static int mouse_rel_x;
static int mouse_rel_y;

void retro_reset(void)
{
   x_coord = 0;
   y_coord = 0;
}

static void update_input(void)
{
   int dir_x = 0;
   int dir_y = 0;
   bool button_X = false;
   bool button_A = false;

   input_poll_cb();
   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))
      dir_y--;
   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))
      dir_y++;
   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))
      dir_x--;
   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))
      dir_x++;
   button_X = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X);
   button_A = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
   

   if (input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RETURN))
      log_cb(RETRO_LOG_INFO, "Return key is pressed!\n");

   if (input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_x))
      log_cb(RETRO_LOG_INFO, "x key is pressed!\n");

   int16_t mouse_x = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
   int16_t mouse_y = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
   bool mouse_l = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
   bool mouse_r = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);
   bool mouse_down = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELDOWN);
   bool mouse_up = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELUP);
   bool mouse_middle = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE);
   if (mouse_x)
      log_cb(RETRO_LOG_INFO, "Mouse X: %d\n", mouse_x);
   if (mouse_y)
      log_cb(RETRO_LOG_INFO, "Mouse Y: %d\n", mouse_y);
   if (mouse_l)
      log_cb(RETRO_LOG_INFO, "Mouse L pressed.\n");
   if (mouse_r)
      log_cb(RETRO_LOG_INFO, "Mouse R pressed.\n");
   if (mouse_down)
      log_cb(RETRO_LOG_INFO, "Mouse wheeldown pressed.\n");
   if (mouse_up)
      log_cb(RETRO_LOG_INFO, "Mouse wheelup pressed.\n");
   if (mouse_middle)
      log_cb(RETRO_LOG_INFO, "Mouse middle pressed.\n");

   mouse_rel_x += mouse_x;
   mouse_rel_y += mouse_y;
   if (mouse_rel_x >= 310)
      mouse_rel_x = 309;
   else if (mouse_rel_x < 10)
      mouse_rel_x = 10;
   if (mouse_rel_y >= 230)
      mouse_rel_y = 229;
   else if (mouse_rel_y < 10)
      mouse_rel_y = 10;

   bool pointer_pressed = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
   int16_t pointer_x = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
   int16_t pointer_y = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
   if (pointer_pressed)
      log_cb(RETRO_LOG_INFO, "Pointer: (%6d, %6d).\n", pointer_x, pointer_y);

   dir_x += input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X) / 5000;
   dir_y += input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y) / 5000;
   dir_x += input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X) / 5000;
   dir_y += input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y) / 5000;

   keyboard_.gamepad_button_left_ = dir_x > 0;
   keyboard_.gamepad_button_right_ = dir_x < 0;
   keyboard_.gamepad_button_top_ = dir_y > 0;
   keyboard_.gamepad_button_bottom_ = dir_y < 0;

   keyboard_.gamepad_button_X_ = button_X;
   keyboard_.gamepad_button_A_ = button_A;

   x_coord = (x_coord + dir_x) & 31;
   y_coord = (y_coord + dir_y) & 31;

   if (rumble.set_rumble_state)
   {
      static bool old_start;
      static bool old_select;
      uint16_t strength_strong = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2) ? 0x4000 : 0xffff;
      uint16_t strength_weak = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2) ? 0x4000 : 0xffff;
      bool start = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START);
      bool select = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT);
      if (old_start != start)
         log_cb(RETRO_LOG_INFO, "Strong rumble: %s.\n", start ? "ON" : "OFF");
      rumble.set_rumble_state(0, RETRO_RUMBLE_STRONG, start * strength_strong);

      if (old_select != select)
         log_cb(RETRO_LOG_INFO, "Weak rumble: %s.\n", select ? "ON" : "OFF");
      rumble.set_rumble_state(0, RETRO_RUMBLE_WEAK, select * strength_weak);

      old_start = start;
      old_select = select;
   }
}



static void check_variables(void)
{
   struct retro_variable var = { 0 };
   var.key = "test_opt0";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      log_cb(RETRO_LOG_INFO, "Key -> Val: %s -> %s.\n", var.key, var.value);
   var.key = "test_opt1";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      log_cb(RETRO_LOG_INFO, "Key -> Val: %s -> %s.\n", var.key, var.value);
   var.key = "test_opt2";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      log_cb(RETRO_LOG_INFO, "Key -> Val: %s -> %s.\n", var.key, var.value);

   float last = last_aspect;
   float last_rate = last_sample_rate;
   struct retro_system_av_info info;
   retro_get_system_av_info(&info);

   if ((last != last_aspect && last != 0.0f) || (last_rate != last_sample_rate && last_rate != 0.0f))
   {
      // SET_SYSTEM_AV_INFO can only be called within retro_run().
      // check_variables() is called once in retro_load_game(), but the checks
      // on last and last_rate ensures this path is never hit that early.
      // last_aspect and last_sample_rate are not updated until retro_get_system_av_info(),
      // which must come after retro_load_game().
      bool ret;
      if (last_rate != last_sample_rate && last_rate != 0.0f) // If audio rate changes, go through SET_SYSTEM_AV_INFO.
         ret = environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &info);
      else // If only aspect changed, take the simpler path.
         ret = environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &info.geometry);
      log_cb(RETRO_LOG_INFO, "SET_SYSTEM_AV_INFO/SET_GEOMETRY = %u.\n", ret);
   }
}

static void audio_callback(void)
{
   for (unsigned i = 0; i < 30000 / 60; i++, phase++)
   {
      int16_t val = 0x800 * sinf(2.0f * M_PI * phase * 300.0f / 30000.0f);
      audio_cb(val, val);
   }

   phase %= 100;
}

static void audio_set_state(bool enable)
{
   (void)enable;
}

void retro_run(void)
{
   // gx4000 for the moment
   //motherboard_->Start(Motherboard::HW_PLUS, 4000 * 50 * 20);
   motherboard_->StartOptimizedPlus<true, false, false>(4000 * 50 * 20);

   update_input();

   if (!use_audio_cb)
      audio_callback();

   bool updated = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables();
}

static void keyboard_cb(bool down, unsigned keycode,
   uint32_t character, uint16_t mod)
{
   log_cb(RETRO_LOG_INFO, "Down: %s, Code: %d, Char: %u, Mod: %u.\n",
      down ? "yes" : "no", keycode, character, mod);
}


bool retro_load_game(const struct retro_game_info *info)
{
   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
      { 0 },
   };

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
      return false;
   }

   struct retro_keyboard_callback cb = { keyboard_cb };
   environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb);
   if (environ_cb(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &rumble))
      log_cb(RETRO_LOG_INFO, "Rumble environment supported.\n");
   else
      log_cb(RETRO_LOG_INFO, "Rumble environment not supported.\n");

   struct retro_audio_callback audio_cb = { audio_callback, audio_set_state };
   use_audio_cb = environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK, &audio_cb);

   check_variables();

   (void)info;
   if (info != nullptr)
   {
      FILE* f;
      unsigned char* buffer_ = nullptr;
      f = fopen(info->path, "rb");
      if (f != nullptr)
      {
         fseek(f, 0, SEEK_END);
         unsigned int buffer_size_ = ftell(f);
         rewind(f);
         unsigned char* buffer_ = new unsigned char[buffer_size_];

         fread(buffer_, buffer_size_, 1, f);
         LoadCprFromBuffer(buffer_, buffer_size_);
         fclose(f);
      }

      motherboard_->GetPSG()->Reset();
      motherboard_->GetSig()->Reset();
      motherboard_->InitStartOptimizedPlus();
      motherboard_->OnOff();
   }

   return true;
}

void retro_unload_game(void)
{
   last_aspect = 0.0f;
   last_sample_rate = 0.0f;
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   if (type != 0x200)
      return false;
   if (num != 2)
      return false;
   return retro_load_game(NULL);
}

size_t retro_serialize_size(void)
{
   return 2;
}

bool retro_serialize(void *data_, size_t size)
{
   if (size < 2)
      return false;

   uint8_t *data = (uint8_t *)data_;
   data[0] = x_coord;
   data[1] = y_coord;
   return true;
}

bool retro_unserialize(const void *data_, size_t size)
{
   if (size < 2)
      return false;

   const uint8_t *data = (const uint8_t *)data_;
   x_coord = data[0] & 31;
   y_coord = data[1] & 31;
   return true;
}

void *retro_get_memory_data(unsigned id)
{
   (void)id;
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   (void)id;
   return 0;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}
