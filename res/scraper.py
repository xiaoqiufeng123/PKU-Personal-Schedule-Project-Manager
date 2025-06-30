# scraper.py (最终版 - 接收文件路径参数)
import json
import sys
from bs4 import BeautifulSoup
import re

def parse_schedule_from_file(file_path):
    """
    从指定的文件路径读取HTML内容并解析。
    """
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            html_content = f.read()
    except FileNotFoundError:
        print(f"错误: 文件未找到 - {file_path}", file=sys.stderr)
        return None
    except Exception as e:
        print(f"错误: 读取文件时发生错误 - {e}", file=sys.stderr)
        return None

    soup = BeautifulSoup(html_content, 'html.parser')
    table = soup.find('table', id='subtable')
    if not table:
        print("错误: 在HTML文件中未找到id为'subtable'的课表。", file=sys.stderr)
        return []

    # --- 解析逻辑保持不变 ---
    rows = table.find_all('tr', class_='ptr_tr')
    grid = [[None for _ in range(10)] for _ in range(15)]
    courses = []
    for row_idx, tr in enumerate(rows):
        real_row = row_idx + 1
        day_pointer = 1
        for td in tr.find_all('td'):
            if td.find('div') and td.find('div').text.isdigit(): continue
            while day_pointer < 8 and grid[real_row][day_pointer] is not None: day_pointer += 1
            if day_pointer > 7: continue
            course_div = td.find('div')
            if course_div and course_div.text.strip():
                rowspan = int(td.get('rowspan', 1))
                lines = [str(line).strip() for line in course_div.contents if str(line).strip()]
                raw_text = ' '.join(lines).replace('<br/>', ' ').replace('<br>', ' ')
                name_match = re.match(r'(.+?)\(', raw_text)
                name = name_match.group(1) if name_match else lines[0]
                classroom, teacher = "未知", "未知"
                info_match = re.search(r'上课信息：.+?\s+(.+?)\s+教师：(.+?)\s+', raw_text)
                if info_match:
                    classroom, teacher = info_match.group(1).strip(), info_match.group(2).strip()
                style = course_div.get('style', '')
                color_match = re.search(r'background-color:\s*(.*?);', style)
                color = color_match.group(1) if color_match else '#FFFFFF'
                course_info = {
                    "name": name, "classroom": classroom, "teacher": teacher,
                    "day": day_pointer, "start_period": real_row,
                    "periods": rowspan, "color": color, "raw_text": raw_text
                }
                courses.append(course_info)
                for i in range(rowspan):
                    if real_row + i < 15 and day_pointer < 10:
                        grid[real_row + i][day_pointer] = name
            day_pointer += 1
    return courses


if __name__ == '__main__':
    # 检查是否从命令行接收到了文件路径参数
    if len(sys.argv) > 1:
        file_path_from_cpp = sys.argv[1]
        print(f"Python: 已接收到文件路径: {file_path_from_cpp}", file=sys.stderr)

        data = parse_schedule_from_file(file_path_from_cpp)

        if data is not None:
            print(f"Python: 解析完成。共找到 {len(data)} 门课程。", file=sys.stderr)
            output_json_string = json.dumps(data, ensure_ascii=False)
            sys.stdout.buffer.write(output_json_string.encode('utf-8'))
        else:
            sys.exit(1) # 如果解析失败，则退出

    else:
        print("错误: 未从C++程序接收到文件路径参数。", file=sys.stderr)
        sys.exit(1)
