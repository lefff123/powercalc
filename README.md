# PowerCalc

Расчёт установившихся режимов ЭС методом Ньютона + редактор отчётов с живым превью и экспортом HTML/PDF.

## Лицензия

MIT — см. [LICENSE](LICENSE). Сторонние библиотеки и их лицензии перечислены в меню «Справка → О программе».

## Сборка (Linux, Ubuntu 24.04+)

```bash
sudo apt install build-essential cmake git python3-pip qpdf pipx \
    qt6-base-dev qt6-webengine-dev qt6-5compat-dev
pipx install conan && conan profile detect

mkdir build && cd build
conan install .. --build=missing
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
ctest
```

Без тестов: `cmake .. -DBUILD_TESTS=OFF …`.

## Сборка (Windows 10/11, с нуля)

1. **Git**: https://git-scm.com/download/win — установка с дефолтами. Проверка: `git --version`.
2. **Visual Studio 2022 Community** (бесплатная): https://visualstudio.microsoft.com/ru/downloads/ — в инсталляторе выбрать workload «Разработка классических приложений на C++» (даст MSVC, Windows SDK, CMake, Ninja).
3. **Qt**: Qt Online Installer (https://www.qt.io/download-qt-installer, нужна бесплатная учётка). В выборе компонентов: Qt → 6.8.x → галки **MSVC 2022 64-bit**, **Qt 5 Compatibility Module**, **Qt WebEngine**. Путь по умолчанию `C:\Qt`.
4. **Python + Conan**: https://python.org (3.12, галка «Add Python to PATH»), затем в PowerShell:

```powershell
pip install conan
conan profile detect
```

5. **Код**:

```powershell
git clone <url-репозитория> powercalc
cd powercalc
```

6. **Сборка** — из меню Пуск открыть «x64 Native Tools Command Prompt for VS 2022» и в ней:

```bat
mkdir build
cd build
conan install .. --build=missing -s build_type=Release
cmake .. -G Ninja -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:\Qt\6.8.2\msvc2022_64
cmake --build .
```

(путь в `CMAKE_PREFIX_PATH` поправь под свою версию Qt).

7. **Рантайм рядом с exe** (Qt-dll, ресурсы WebEngine):

```bat
C:\Qt\6.8.2\msvc2022_64\bin\windeployqt.exe RELEASE\powercalc.exe
```

8. **qpdf** (нужен для нумерации страниц в PDF):

```powershell
powershell -ExecutionPolicy Bypass -File scripts\fetch-qpdf.ps1
xcopy /E /Y 3rdparty\qpdf\bin\* build\RELEASE\
```

9. Запуск: `RELEASE\powercalc.exe`. Тесты: `ctest --test-dir . -C Release`.

Папка `build\RELEASE` (exe + dll + qpdf) — готовый дистрибутив.

## Windows: чёрный экран / краш превью

Перед запуском:

```bat
set QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu
set QT_OPENGL=software
```

Если всё равно крашится:

```bat
set QTWEBENGINE_DISABLE_SANDBOX=1
```

## Синтаксис документа (v1.3)

**YAML-мета**: `title/author/date`, `show_substitution`, `align`, `heading_align`,
`page.size` (A4/A3/A5/Letter), `page.margin` (cm/mm), `text.size` (pt),
`show_page_numbers`, `page_start`, `number_first_page`, `toc_size`.

**Формулы-блоки**: любой текст между открывающими и закрывающими `$$` (в одну строку или в несколько) — формула.
- `$$R = 10 &Ом$$` — присваивание; единица после `&` — серым;
- `$$R = \frac{u}{i} &Ом$$` — при включённой подстановке показывает цепочку `= 10/2 = 5`;
- `$$u =$$` — текущее значение переменной;
- `$$u$$` / `$$u/2+1$$` — голое выражение: подставляется значение;
- `$$! …$$` (слитно или с пробелом, в т.ч. с первой строки содержимого) — чистый LaTeX: рисуется ровно как написано, не считается;
- `$$hide … $$` — блок скрыт, но считается и определяет переменные; `$$hide! …$$` — скрыт и не считается;
- `# комментарий` внутри блока — не выводится;
- локальный стиль после закрывающего `$$`: `$$x = 1$$ {align:center,size:12pt,show_substitution:false}`.

**Inline в тексте**: те же правила: `$$u/2$$` → значение, `$$Z = R+1$$` — определяет переменную, `$$!…$$` — чистый LaTeX; единицы `&…` работают.

**Подстановка**: глобально `show_substitution`, локально `{show_substitution:true/false}`; тривиальные присваивания (`u = 10`) значением не дублируются.

**Таблицы**: ячейки `$$u/2$$`, `$$u=$$` вычисляются; `$$!…$$` — чистый LaTeX;
разделитель `|:--:|` задаёт выравнивание колонки; таблица центрирована.

**Списки**: `- ` и `1. `, вложенность отступом 2 пробела, до 3 уровней.

**Картинки**: `![подпись](файл.png)` — файл в images/ проекта (внутри ZIP); блок картинки не разрывается между страницами.
Вставка: «Вставка → Картинка…», drag&drop файла, Ctrl+V со скриншотом.

**Оглавление**: `[toc]` — точки и номера страниц как в Word, ссылки кликабельны; заголовок, стоящий непосредственно перед `[toc]` (например `# Оглавление`), в список не попадает; размер шрифта — `toc_size: 12pt` в YAML.

**Разрыв страницы**: `[break]` (или `[pagebreak]`) — следующий блок начинается с новой страницы.

**Нумерация страниц** (только при экспорте в PDF; требуется qpdf): `show_page_numbers: true/false`, `page_start: N` — номер первой нумерованной страницы, `number_first_page: true/false` — нумеровать ли первую страницу. Номер печатается внизу по центру в нижнем поле.

**Заголовки**: `#`/`##`/`###`; локально `# Титл {center,12pt}` (или `{align:center,size:12pt}`).