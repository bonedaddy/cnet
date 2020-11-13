.PHONY: format
format:
	find . -type f -name "*.c" -not -name "*_test.c" -exec clang-format -i {} \;
	find . -type f -name "*.h" -exec clang-format -i {} \;

