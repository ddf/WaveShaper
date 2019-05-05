#define PLUG_NAME "WaveShaper"
#define PLUG_MFR "Damien Quartz"
#define PLUG_VERSION_HEX 0x00010000
#define PLUG_VERSION_STR "1.0.0"
#define PLUG_UNIQUE_ID 'Wvsh'
#define PLUG_MFR_ID 'Cmpt'
#define PLUG_URL_STR "http://www.waveshaperapp.com/"
#define PLUG_EMAIL_STR "info@compartmental.net"
#define PLUG_COPYRIGHT_STR "Copyright 2019 Damien Quartz"
#define PLUG_CLASS_NAME WaveShaper

#define BUNDLE_NAME "WaveShaper"
#define BUNDLE_MFR "compartmental"
#define BUNDLE_DOMAIN "net"

#define PLUG_CHANNEL_IO "0-2"

#define PLUG_LATENCY 0
#define PLUG_TYPE 1
#define PLUG_DOES_MIDI_IN 1
#define PLUG_DOES_MIDI_OUT 1
#define PLUG_DOES_MPE 1
#define PLUG_DOES_STATE_CHUNKS 0
#define PLUG_HAS_UI 1
#define PLUG_WIDTH 1024
#define PLUG_HEIGHT 669
#define PLUG_FPS 60
#define PLUG_SHARED_RESOURCES 0

#define AUV2_ENTRY WaveShaper_Entry
#define AUV2_ENTRY_STR "WaveShaper_Entry"
#define AUV2_FACTORY WaveShaper_Factory
#define AUV2_VIEW_CLASS WaveShaper_View
#define AUV2_VIEW_CLASS_STR "WaveShaper_View"

#define AAX_TYPE_IDS 'EFN1', 'EFN2'
#define AAX_PLUG_MFR_STR "Acme"
#define AAX_PLUG_NAME_STR "WaveShaper\nIPIS"
#define AAX_DOES_AUDIOSUITE 0
#define AAX_PLUG_CATEGORY_STR "Synth"

#define VST3_SUBCATEGORY "Instrument|Synth"

#define APP_NUM_CHANNELS 2
#define APP_N_VECTOR_WAIT 0
#define APP_MULT 1
#define APP_COPY_AUV3 0
#define APP_RESIZABLE 0
#define APP_SIGNAL_VECTOR_SIZE 64

#define ROBOTTO_FN "Roboto-Regular.ttf"
