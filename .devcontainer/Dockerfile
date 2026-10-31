# 1. Берем вашу рабочую ОС с LaTeX за основу
FROM texlive/texlive:latest

# 2. Доставляем C++ утилиты через встроенный менеджер пакетов (Debian)
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    gdb \
    clang \
    git \
    openssh-client \
    curl \
    clang-format \
    inkscape \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# В образе texlive по умолчанию всё работает под root. 
# VS Code автоматически адаптирует права, поэтому пользователя можно не менять.
