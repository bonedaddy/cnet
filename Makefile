.PHONY: format
format:
	find . -type f -name "*.c" -not -path "./deps/*" -not -name "*_test.c" -exec clang-format -i {} \;
	find . -type f -name "*.h" -not -path "./deps/*" -exec clang-format -i {} \;

