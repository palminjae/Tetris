# 크로스플랫폼 테트리스 Makefile
# Windows (MinGW/MSYS2/Cygwin)와 Unix/Linux/macOS 모두 지원

# 컴파일러 설정
CC = gcc

# 기본 컴파일 플래그
CFLAGS = -Wall -Wextra -std=c99 -O2

# 소스 파일
SRCFILE = tetris.c

# 플랫폼 감지
ifeq ($(OS),Windows_NT)
    # Windows 환경
    TARGET = tetris.c
    PLATFORM = Windows
    # Windows용 추가 플래그 (필요시)
    LDFLAGS = 
    RM = del /Q
    CLEAN_TARGET = tetris.exe tetris_result.dat
else
    # Unix/Linux/macOS 환경
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        PLATFORM = Linux
    endif
    ifeq ($(UNAME_S),Darwin)
        PLATFORM = macOS
    endif
    ifndef PLATFORM
        PLATFORM = Unix
    endif
    
    TARGET = tetris
    LDFLAGS = 
    RM = rm -f
    CLEAN_TARGET = tetris tetris_result.dat
endif

# 기본 타겟
.PHONY: all clean run help install debug release

all: $(TARGET)
	@echo "==================================="
	@echo "빌드 완료!"
	@echo "플랫폼: $(PLATFORM)"
	@echo "실행파일: $(TARGET)"
	@echo "==================================="

# 메인 빌드 룰
$(TARGET): $(SRCFILE)
	@echo "==================================="
	@echo "$(PLATFORM)용 테트리스 컴파일 중..."
	@echo "==================================="
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCFILE) $(LDFLAGS)

# 디버그 빌드
debug: CFLAGS += -DDEBUG -g
debug: $(TARGET)
	@echo "디버그 빌드 완료"

# 릴리즈 빌드 (최적화 강화)
release: CFLAGS += -DNDEBUG -O3
release: $(TARGET)
	@echo "릴리즈 빌드 완료"

# 실행
run: $(TARGET)
	@echo "==================================="
	@echo "테트리스 게임 실행 중..."
	@echo "==================================="
ifeq ($(OS),Windows_NT)
	./$(TARGET)
else
	./$(TARGET)
endif

# 정리
clean:
	@echo "==================================="
	@echo "빌드 파일 정리 중..."
	@echo "==================================="
	$(RM) $(CLEAN_TARGET)
	@echo "정리 완료"

# 도움말
help:
	@echo "==================================="
	@echo "크로스플랫폼 테트리스 Makefile"
	@echo "==================================="
	@echo "사용 가능한 명령어:"
	@echo ""
	@echo "  make 또는 make all    - 일반 빌드"
	@echo "  make debug           - 디버그 빌드 (-g 플래그 포함)"
	@echo "  make release         - 릴리즈 빌드 (최적화 강화)"
	@echo "  make run             - 빌드 후 실행"
	@echo "  make clean           - 빌드 파일 삭제"
	@echo "  make install         - 시스템에 설치"
	@echo "  make help            - 이 도움말 표시"
	@echo ""
	@echo "현재 플랫폼: $(PLATFORM)"
	@echo "타겟 파일: $(TARGET)"
	@echo "==================================="

# 설치 (선택적)
install: $(TARGET)
ifeq ($(OS),Windows_NT)
	@echo "Windows에서는 수동으로 PATH에 추가하세요"
	@echo "또는 원하는 위치로 $(TARGET) 파일을 복사하세요"
else
	@echo "시스템에 설치 중..."
	sudo cp $(TARGET) /usr/local/bin/
	@echo "설치 완료! 어디서든 'tetris' 명령으로 실행 가능합니다"
endif

# 컴파일러 및 플래그 정보 표시
info:
	@echo "==================================="
	@echo "빌드 환경 정보"
	@echo "==================================="
	@echo "컴파일러: $(CC)"
	@echo "컴파일 플래그: $(CFLAGS)"
	@echo "링크 플래그: $(LDFLAGS)"
	@echo "소스 파일: $(SRCFILE)"
	@echo "타겟 파일: $(TARGET)"
	@echo "플랫폼: $(PLATFORM)"
	@echo "==================================="

# 빠른 테스트 빌드 및 실행
test: clean all run

# 의존성 체크 (고급 사용자용)
check:
	@echo "==================================="
	@echo "시스템 의존성 확인 중..."
	@echo "==================================="
	@echo -n "GCC 버전: "
	@$(CC) --version | head -n1 || echo "GCC가 설치되지 않았습니다"
	@echo -n "Make 버전: "
	@make --version | head -n1 || echo "Make가 설치되지 않았습니다"
ifeq ($(OS),Windows_NT)
	@echo "Windows 환경 감지됨"
else
	@echo -n "시스템: "
	@uname -a
endif
	@echo "==================================="