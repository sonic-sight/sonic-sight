CXX=g++
REMOTEHOST=sonicthree
REMOTEHOST=sonichotspot
CPPSRC=$(wildcard src/*.cpp)
CSRC=$(wildcard src/*.c)
SRC=$(CPPSRC) $(CSRC)
# SRC=$(wildcard src/*.cpp)

# Each application has its .cpp file with its main function
APPLICATIONS=render-to-sound test-sound-generation
MAINSRCS=$(patsubst %,src/%.cpp,$(APPLICATIONS))
NOMAINSRCS=$(filter-out $(MAINSRCS),$(SRC))

.PHONY : all both push pulldata pullimages pullsounds ompdebug ompcompare remoteclean remotetest test localrun \
	rpi-set-soundcard-internal rpi-set-soundcard-usb
.DEFAULT_GOAL = all

RELEASEBUILD ?=DEBUG
DEBUGOMP?=0

TARGETNAME_DEBUG=DEBUG
TARGETNAME_RELEASE=RELEASE

DIR_ALL=
DIR_DEBUG=debug_build
DIR_RELEASE=release_build
DIR=$(DIR_$(RELEASEBUILD))

ALLBUILDDIRS=$(DIR_DEBUG) $(DIR_RELEASE)

OBJ=$(addprefix $(DIR)/,$(patsubst %.cpp, %.o, $(CPPSRC) ) $(patsubst %.c, %.o, $(CSRC) ) )
MAINOBJ=$(addprefix $(DIR)/,$(patsubst %.cpp, %.o, $(MAINSRCS) ))
NOMAINOBJ=$(filter-out $(MAINOBJ),$(OBJ))

CPPFLAGS_ALL= -c -fmessage-length=0 -static -std=c++11 -I/usr/local/include -DDEBUGOMP=$(DEBUGOMP)
CPPFLAGS_DEBUG= -O0 -g3
CPPFLAGS_RELEASE= -DNDEBUG -O1 -fopenmp
CPPFLAGS=$(CPPFLAGS_ALL) $(CPPFLAGS_$(RELEASEBUILD))

LDFLAGS_ALL=-L/usr/local/lib -lpthread -lrealsense2 -lSDL2
LDFLAGS_DEBUG=
LDFLAGS_RELEASE= -fopenmp
LDFLAGS=$(LDFLAGS_ALL) $(LDFLAGS_$(RELEASEBUILD))

$(info $$SRC $(SRC))
$(info $$NOMAINSRCS $(NOMAINSRCS))
$(info $$MAINSRCS $(MAINSRCS))
$(info $$OBJ $(OBJ))
$(info $$NOMAINOBJ $(NOMAINOBJ))
LASTDEPTH=$(lastword $(sort $(wildcard depth/*.dat ) ) )
LASTWAVEFORM=$(lastword $(sort $(wildcard depth/*.waveform ) ) )
$(info $$LASTDEPTH $(LASTDEPTH))
# $(error exiting)


debug_build release_build :
	mkdir "$@"

debug : | debug_build
	export RELEASEBUILD=$(TARGETNAME_DEBUG); make all

release : | release_build
	export RELEASEBUILD=$(TARGETNAME_RELEASE); make all

both : debug release

$(DIR)/%.o: %.cpp Makefile
	@-mkdir -p $(shell dirname $@) &> /dev/null
	$(CXX) -c $< $(CPPFLAGS) $(LDFLAGS) $(INCLUDEPATH) $(LDFLAGS) -o $@ 
	$(CXX) -MM $(CPPFLAGS) $(LDFLAGS) $(INCLUDEPATH) -c $< $(LDFLAGS) > $(patsubst %.o,%.d,$@)

$(DIR)/%.o: %.c Makefile
	@-mkdir -p $(shell dirname $@) &> /dev/null
	$(CC) -c $< $(CPPFLAGS) $(LDFLAGS) $(INCLUDEPATH) $(LDFLAGS) -o $@ 
	$(CC) -MM $(CPPFLAGS) $(LDFLAGS) $(INCLUDEPATH) -c $< $(LDFLAGS) > $(patsubst %.o,%.d,$@)

$(addprefix $(DIR)/,$(APPLICATIONS)) : $(OBJ)
	@echo "Building application $@"
	$(CXX) $(LDFLAGS) $(INCLUDEPATH) -o $@ $(NOMAINOBJ) $(patsubst $(DIR)/%,$(DIR)/src/%.o,$@) $(LDFLAGS)

all : $(addprefix $(DIR)/,$(APPLICATIONS))
	@echo "Building target $@"
	@echo "Building target $(TARGETNAME)"

clean :
	-rm -rf $(DIR_DEBUG)
	-rm -rf $(DIR_RELEASE)
	-rm -rf data/*

push :
	rsync -C --delete -av --progress --exclude='.*.swp' --exclude='/data' --exclude='/test-data' --exclude='/debug_build' --exclude='/release_build' ./ sonic@$(REMOTEHOST):/home/sonic/sonic-sight/

pulldata : 
	rsync -av --progress sonic@$(REMOTEHOST):/home/sonic/sonic-sight/data/ ./data

pullimages : 
	rsync -av --include='*.png' --exclude='*' --progress sonic@$(REMOTEHOST):/home/sonic/sonic-sight/data/ ./data

pullsounds : 
	rsync -av --progress sonic@$(REMOTEHOST):/home/sonic/sonic-sight/scripts/sounds/ ./scripts/sounds

ompdebug :
	DEBUGOMP=1 make clean debug release
	./debug_build/render-to-sound --camera-fps=6
	mv debug_data.dat debug_data_DEBUG.dat
	./release_build/render-to-sound --camera-fps=6
	mv debug_data.dat debug_data_RELEASE.dat
	make ompcompare

ompcompare:
	../scripts/compare-debug-data.py debug_data_DEBUG.dat debug_data_RELEASE.dat
	# ipython3 -i ../scripts/compare-debug-data.py -- debug_data_DEBUG.dat debug_data_RELEASE.dat
	
remoteclean :
	ssh -t -t sonic@$(REMOTEHOST) 'cd /home/sonic/sonic-sight ; make clean'

remotetest :
	make push
	ssh -t -t sonic@$(REMOTEHOST) 'cd /home/sonic/sonic-sight ; make release'
	ssh -t -t sonic@$(REMOTEHOST) 'systemctl --user restart sonic-sight.service'

remotestop :
	ssh -t -t sonic@$(REMOTEHOST) 'systemctl --user stop sonic-sight.service'

remotestart :
	ssh -t -t sonic@$(REMOTEHOST) 'systemctl --user start sonic-sight.service'

remotewatch :
	( while [ 1 ] ; do ssh -t -t sonic@$(REMOTEHOST) screen -dRR sonic-sight ; sleep 1 ; done )

test :
	make push
	# ssh -t -t sonic@$(REMOTEHOST) 'cd /home/sonic/sonic-sight/scripts ; python3 -i ./button-test.py'
	ssh -t -t sonic@$(REMOTEHOST) 'cd /home/sonic/sonic-sight/scripts ; python3 -i ./button-menu.py'
	make pullsounds

plot :
	../scripts/plot-distance-image.py --image-width=640 --image-height=480 --show $(LASTDEPTH)
	../scripts/plot-wave-file.py $(LASTWAVEFORM)
	 

rpi-set-soundcard-internal :
	pulseaudio --check || pulseaudio -D
	pacmd set-default-sink 0

rpi-set-soundcard-usb :
	pulseaudio --check || pulseaudio -D
	pacmd set-default-sink 1
