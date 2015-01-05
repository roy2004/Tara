BUILD_DIR = Build
SOURCE_DIR = Source
CPPFLAGS = -MMD -MT $@ -MF $(BUILD_DIR)/$*.d
CXXFLAGS = -std=c++11 -Wno-invalid-offsetof

build: $(BUILD_DIR)/a.out

clean:
	$(RM) $(BUILD_DIR)/*

ifneq ($(MAKECMDGOALS),clean)
-include $(patsubst $(SOURCE_DIR)/%.cxx,$(BUILD_DIR)/%.d,$(wildcard $(SOURCE_DIR)/*.cxx))
endif

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.cxx
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/a.out: $(patsubst $(SOURCE_DIR)/%.cxx,$(BUILD_DIR)/%.o,$(wildcard $(SOURCE_DIR)/*.cxx))
	$(CXX) $(LDFLAGS) -o $@ $^
