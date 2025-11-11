# Генератор грамот

Небольшая консольная утилита на C++, которая автоматизирует заполнение шаблона грамоты
по данным из CSV-файла. Программа читает текстовый шаблон, список награждаемых и файл со
стилями оформления, после чего формирует персонализированные PDF-грамоты и сводный отчет.

## Требования

* Компилятор с поддержкой стандарта C++17

## Структура данных

* `data/certificate_template.txt` — текстовый шаблон грамоты. Поддерживаемые плейсхолдеры:
  * `{FULL_NAME}` — ФИО награждаемого
  * `{PLACE}` — занятое место
  * `{ACHIEVEMENT}` — описание достижения
  * `{DATE}` — дата выдачи
  * `{NOTES}` — дополнительные примечания
  * `{BACKGROUND}`, `{FONT}`, `{STYLE_NAME}` — сведения о выбранном стиле
* `data/recipients.csv` — входные данные в формате CSV с разделителем `;` и заголовком. Обязательные столбцы: `ФИО`, `Место`, `Достижение`, `Дата`, `Примечания`.
* `styles/*.txt` — описание визуальных стилей. Каждый файл содержит пары `ключ:значение`
  с полями `background` и `font`. Название стиля совпадает с именем файла.

## Сборка

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

### Где искать исполнимый файл

* При использовании одноконфигурационных генераторов CMake (Ninja, MinGW Makefiles)
  бинарник появляется прямо в каталоге `build`. На Windows он будет называться
  `certificate_tool.exe`. Запускайте его из каталога сборки (`./certificate_tool` или
  `.\\certificate_tool.exe`) либо укажите полный путь из корня проекта
  (`.\\build\\certificate_tool.exe`).
* Если CMake создаёт многофайловую конфигурацию Visual Studio, укажите нужный режим
  сборки: `cmake --build . --config Release`. Исполнимый файл появится в подпапке
  `Release` (или `Debug`), например `.\\build\\Release\\certificate_tool.exe`.

Для простоты можно воспользоваться и прямым вызовом компилятора:

```bash
g++ -std=c++17 -Isrc -Iinclude src/*.cpp -o certificate_tool
```

## Запуск

```bash
./certificate_tool \
  --template data/certificate_template.txt \
  --data data/recipients.csv \
  --styles styles \
  --style classic \
  --output output
```

### Запуск в PowerShell

В PowerShell символ `\` не работает как перенос строки. Либо выполните команду в одну
строку, либо используйте обратный апостроф `` ` `` для продолжения строки:

```powershell
# если вы остались внутри каталога build после сборки
Set-Location build
.\certificate_tool.exe `
  --template ..\data\certificate_template.txt `
  --data ..\data\recipients.csv `
  --styles ..\styles `
  --style classic `
  --output ..\output

# если выполняете команду из корня репозитория
.\build\certificate_tool.exe `
  --template data\certificate_template.txt `
  --data data\recipients.csv `
  --styles styles `
  --style classic `
  --output output
```

Убедитесь, что исполнимый файл лежит в папке `build` (или `build\Release` при
многофайловой конфигурации Visual Studio). При желании можно скопировать
`certificate_tool.exe` в корень проекта — тогда команда из примера выше без пути
(`.\certificate_tool.exe`) тоже будет работать.

По завершении работы утилита выведет в консоль ФИО и место каждого награжденного, создаст PDF-файлы грамот в указанной папке и сформирует отчет `metadata.csv` с деталями по каждому участнику.

> ⚠️ Если программа сообщает `Styles directory not found: ...`, значит путь, переданный в `--styles`, указывает на несуществующую папку. Убедитесь, что вы запускаете утилиту из нужного каталога (например, `build`) и скорректируйте путь на `..\styles` или `../styles`.
