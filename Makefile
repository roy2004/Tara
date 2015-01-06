BUILD_DIR = Build
SOURCE_DIR = Source
CPPFLAGS = -MMD -MT $@ -MF $(BUILD_DIR)/$*.d
CXXFLAGS = -std=c++11 -Wno-invalid-offsetof

build: $(BUILD_DIR)/a.out

clean:
	$(RM) $(BUILD_DIR)/*

ifneq ($(MAKECMDGOALS), clean)
-include $(patsubst %.cxx, $(BUILD_DIR)/%.d, $(filter %.cxx, $(shell cat SourceList)))
endif

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.cxx
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/a.out: $(patsubst %.cxx, $(BUILD_DIR)/%.o, $(filter %.cxx, $(shell cat SourceList)))
	$(CXX) $(LDFLAGS) -o $@ $^
