CC = psxsdkserial-gcc
DEFINE= -D_PAL_MODE_
DEFINE += -DPSXSDK_DEBUG
LIBS=-lfixmath
CC_FLAGS = -Wall -Werror -c -Os -Wfatal-errors -g
LINKER = psxsdkserial-gcc

PROJECT = OPENSEND
PROJECT_DIR = ~/OpenSend

INIT_ADDR=0x801A0000

ELF2EXE = elf2exe
ELF2EXE_FLAGS = -mark="Open-source PSX-EXE loader created with PSXSDK" -init_addr=$(INIT_ADDR)
LICENSE_FILE = /usr/local/psxsdk/share/licenses/infoeur.dat

PSXSDK_DIR = /usr/local/psxsdk/bin

EMULATOR_DIR = ~/pcsxr
EMULATOR = pcsxr.exe
SOUND_INTERFACE =
EMULATOR_FLAGS = -nogui -psxout
OBJ_DIR = Obj
SRC_DIR = .
MUSIC_TRACKS =
#FFMPEG = ffmpeg
#FFMPEG_DIR = ../Music/ffmpeg/bin
#FFMPEG_FLAGS = -f s16le -acodec pcm_s16le

GNU_SIZE = mipsel-unknown-elf-size

all: build image clean
#emulator clean

rebuild: remove build

build: clean objects $(PROJECT).elf $(PROJECT).exe
	
objects: 	$(addprefix $(OBJ_DIR)/,main.o System.o Gfx.o \
			LoadMenu.o EndAnimation.o			\
			Font.o Serial.o)
			
remove:
	rm -f Obj/*.o
	
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $< -o $@ $(DEFINE) $(CC_FLAGS)
	
$(PROJECT).elf: 
	$(LINKER) Obj/*.o -o Exe/$(PROJECT).elf $(LIBS) -Wl,--gc-sections
	
$(PROJECT).exe:
	$(ELF2EXE) Exe/$(PROJECT).elf Exe/$(PROJECT).exe $(ELF2EXE_FLAGS)
	cp Exe/$(PROJECT).exe ../cdimg
	
image:
	rm -f $(PROJECT).iso $(PROJECT).bin
	rm -f $(PROJECT).cue
	mkisofs -o $(PROJECT).iso -V $(PROJECT) -sysid PLAYSTATION ../cdimg
	mkpsxiso $(PROJECT).iso $(PROJECT).bin $(LICENSE_FILE)
	mv $(PROJECT).bin ../Bin
	mv $(PROJECT).cue ../Bin
	rm -f $(PROJECT).cue
	rm -f $(PROJECT).iso
	$(GNU_SIZE) Exe/$(PROJECT).elf
	
emulator:
	export PATH=$$PATH:$(EMULATOR_DIR)
	$(EMULATOR) -cdfile $(PROJECT_DIR)/Bin/$(PROJECT).bin $(EMULATOR_FLAGS)
	
clean:
	rm -f $(PROJECT).elf cdimg/$(PROJECT).exe $(PROJECT).bin $(PROJECT).cue cdimg/README.txt
	rm -f $(PROJECT).iso $(PROJECT).exe $(PROJECT).elf

