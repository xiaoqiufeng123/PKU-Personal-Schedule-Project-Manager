#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys, json, requests, os

def send_output(payload_str: str):
    """
    辅助函数，用于将字符串负载可靠地编码为UTF-8并写入标准输出。
    """
    sys.stdout.buffer.write(payload_str.encode('utf-8'))
    sys.stdout.flush()

def get_free_classroom(building: str, time: str):
    """
    通过北大门户API获取指定教学楼和时间的空闲教室列表。
    """
    url = "https://portal.pku.edu.cn/publicQuery/classroomQuery/retrClassRoomFree.do"
    params = {"buildingName": building, "time": time}
    headers = {
        "User-Agent": "Mozilla/5.0",
        "Referer": "https://portal.pku.edu.cn/publicQuery/#/freeClassroom",
    }
    # 发起网络请求，设置10秒超时
    resp = requests.get(url, params=params, headers=headers, timeout=10)
    # 如果请求失败（如404, 500错误），则会抛出异常
    resp.raise_for_status()
    # 返回响应JSON中的 "rows" 字段，如果不存在则返回空列表
    return resp.json().get("rows", [])

def write_debug_json(payload: str):
    """
    将查询结果写入一个JSON文件，方便调试。
    """
    # 确保调试文件写在脚本所在的目录
    script_dir = os.path.dirname(os.path.realpath(__file__))
    out_path = os.path.join(script_dir, "last_query.json")
    try:
        # 使用 'utf-8-sig' 编码可以让Windows下的记事本正确显示中文
        with open(out_path, "w", encoding="utf-8-sig") as f:
            f.write(payload)
    except Exception as e:
        # 如果写入失败，在标准错误流中打印调试信息
        print(f"调试: 写入调试文件失败: {e}", file=sys.stderr)

def main():
    # 检查命令行参数是否正确
    if len(sys.argv) != 3:
        payload = json.dumps({"error": "用法: query_free_rooms.py <教学楼> <时间>"}, ensure_ascii=False)
        write_debug_json(payload)
        send_output(payload) # 使用辅助函数输出
        sys.exit(0)

    building, time = sys.argv[1], sys.argv[2]
    try:
        # 尝试获取空闲教室数据
        rows = get_free_classroom(building, time)
    except Exception as e:
        # 当发生异常时（如网络错误），返回一个空的JSON结构体，以防止C++程序崩溃。
        payload = json.dumps({building: {}}, ensure_ascii=False)
        write_debug_json(payload)
        send_output(payload) # 使用辅助函数输出
        sys.exit(0)

    # 构造C++程序所期望的JSON结构
    result = {building: {}}
    for row in rows:
        room = row.get("room", "").strip()
        date = row.get("date", "default")
        if not room:
            continue
        
        # setdefault会返回指定键的值，如果键不存在，则插入带有默认值的键
        sects = result[building].setdefault(date, {})
        for k, v in row.items():
            # 过滤掉非节次信息（如教室名、容量、日期）
            if k in ("room", "cap", "date"):
                continue
            # 如果值为""，表示该节次是空闲的
            if v == "":
                sects.setdefault(k, []).append(room)

    # 将最终结果转换为JSON格式的字符串
    payload = json.dumps(result, ensure_ascii=False)
    write_debug_json(payload)

    # 使用辅助函数进行最终的输出
    send_output(payload)

if __name__ == "__main__":
    main()