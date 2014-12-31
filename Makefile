BUILD_DIR = Build
SOURCE_DIR = Source
CXXFLAGS = -std=c++11 -Wno-invalid-offsetof

build: $(BUILD_DIR)/a.out

clean:
	$(RM) $(BUILD_DIR)/*

ifneq ($(MAKECMDGOALS), clean)
-include $(patsubst $(SOURCE_DIR)/%.cxx, $(BUILD_DIR)/%.d, $(wildcard $(SOURCE_DIR)/*.cxx))
endif

$(BUILD_DIR)/%.d: $(SOURCE_DIR)/%.cxx
	$(CXX) $(CPPFLAGS) -MM -MT $(<:$(SOURCE_DIR)/%.cxx=$(BUILD_DIR)/%.o) -MF $@ $<

$(BUILD_DIR)/%.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/a.out: $(patsubst $(SOURCE_DIR)/%.cxx, $(BUILD_DIR)/%.o, $(wildcard $(SOURCE_DIR)/*.cxx))
	$(CXX) $(LDFLAGS) -o $@ $^
