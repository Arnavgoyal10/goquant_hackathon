# Curve DEX Limit Order Agent Makefile

CXX = g++
CXXFLAGS = -std=c++17 -I/opt/homebrew/include -Wall -Wextra
LDFLAGS = -lcurl
BUILD_DIR = build
SRC_DIR = src

# Main target - this is what you'll run!
main: $(BUILD_DIR)/curve_dex_limit_order_agent
	@echo "🎯 Main program compiled successfully!"
	@echo "Run with: ./$(BUILD_DIR)/curve_dex_limit_order_agent"

# Pool discovery tool - finds real Curve pools on Sepolia
discover_pools: $(BUILD_DIR)/discover_pools
	@echo "🔍 Pool discovery tool compiled!"
	@echo "Run with: ./$(BUILD_DIR)/discover_pools"

$(BUILD_DIR)/discover_pools: $(SRC_DIR)/discover_pools.cpp include/sepolia_config.h
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/discover_pools.cpp -o $@ $(LDFLAGS)

$(BUILD_DIR)/curve_dex_limit_order_agent: $(SRC_DIR)/curve_dex_limit_order_agent.cpp include/limit_order.h
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/curve_dex_limit_order_agent.cpp -o $@ $(LDFLAGS)

# Individual test programs
price_monitor: $(BUILD_DIR)/price_monitor
	./$(BUILD_DIR)/price_monitor

$(BUILD_DIR)/price_monitor: $(SRC_DIR)/price_monitor.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/price_monitor.cpp -o $@ $(LDFLAGS)

wallet_info: $(BUILD_DIR)/wallet_info
	./$(BUILD_DIR)/wallet_info

$(BUILD_DIR)/wallet_info: $(SRC_DIR)/wallet_info.cpp include/sepolia_config.h
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/wallet_info.cpp -o $@ $(LDFLAGS)



# Unit tests
unit_tests: $(BUILD_DIR)/unit_tests
	./$(BUILD_DIR)/unit_tests

$(BUILD_DIR)/unit_tests: tests/unit_tests.cpp include/limit_order.h include/transaction_signer.h
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) tests/unit_tests.cpp -o $@

# End-to-end tests
e2e_tests: $(BUILD_DIR)/e2e_tests
	./$(BUILD_DIR)/e2e_tests

$(BUILD_DIR)/e2e_tests: tests/e2e_tests.cpp include/limit_order.h include/transaction_signer.h
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) tests/e2e_tests.cpp -o $@ $(LDFLAGS)

# Run all tests
test_all: unit_tests e2e_tests
	@echo "🎉 ALL TESTS COMPLETED!"

# Complete system verification
verify: main unit_tests e2e_tests
	@echo ""
	@echo "🚀 COMPLETE SYSTEM VERIFICATION"
	@echo "==============================="
	@echo "✅ Main program compiled"
	@echo "✅ Unit tests passed"
	@echo "✅ E2E tests passed"
	@echo "✅ System ready for deployment!"

# Clean build files
clean:
	rm -rf $(BUILD_DIR)

# Show available targets
help:
	@echo "Available targets:"
	@echo "  main              - Compile and show info for main program"
	@echo "  price_monitor     - Test price monitoring system"
	@echo "  limit_order_test  - Test limit order structure"
	@echo "  sepolia_test      - Test Sepolia connection"
	@echo "  unit_tests        - Run comprehensive unit tests"
	@echo "  e2e_tests         - Run end-to-end integration tests"
	@echo "  test_all          - Run all tests"
	@echo "  verify            - Complete system verification"
	@echo "  clean             - Clean build files"
	@echo ""
		@echo "🎯 To run the final program:"
	@echo "  make main && ./build/curve_dex_limit_order_agent"
	@echo ""
	@echo "📖 CLI Examples:"
	@echo "  ./build/curve_dex_limit_order_agent                    # GTC (default)"
	@echo "  ./build/curve_dex_limit_order_agent 0xPool 1 0 1000000 GTC"
	@echo "  ./build/curve_dex_limit_order_agent 0xPool 1 0 1000000 GTT 30"
	@echo "  ./build/curve_dex_limit_order_agent 0xPool 1 0 1000000 IOC"
	@echo "  ./build/curve_dex_limit_order_agent 0xPool 1 0 1000000 FOK"
	@echo "  TIF_POLICY=IOC ./build/curve_dex_limit_order_agent     # Environment variable"
	@echo ""
	@echo "🧪 To run all tests:"
	@echo "  make verify"

.PHONY: main price_monitor limit_order_test sepolia_test unit_tests e2e_tests test_all verify clean help
