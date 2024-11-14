import os
import argparse

def generate_dat_file(file_path, size_in_bytes):
    """
    生成指定字节大小的 .dat 文件。
    
    :param file_path: 输出文件的路径
    :param size_in_bytes: 文件的大小，单位为字节
    """
    with open(file_path, 'wb') as file:
        # 生成指定大小的内容，初始化为全零字节
        file.write(os.urandom(size_in_bytes))
    print(f"文件 '{file_path}' 已生成，大小为 {size_in_bytes} 字节。")


def main():
    parser = argparse.ArgumentParser(description="生成指定字节大小的 .dat 文件。")
    parser.add_argument("file_path", type=str, help="输出 .dat 文件的路径")
    parser.add_argument("size", type=int, help=".dat 文件的大小（字节）")
    
    args = parser.parse_args()
    generate_dat_file(args.file_path, args.size)

if __name__ == "__main__":
    main()
