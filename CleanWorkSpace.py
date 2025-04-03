import os
import shutil

def delete_log_files(directory):
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith('.log'):
                file_path = os.path.join(root, file)
                try:
                    os.remove(file_path)
                    print(f"已删除文件: {file_path}")
                except Exception as e:
                    print(f"删除文件 {file_path} 时出错: {e}")

def clean_build_folder(directory):
    build_dir = os.path.join(directory, "build")  # 获取 build 文件夹路径
    
    if not os.path.exists(build_dir):
        print(f"⚠️ {build_dir} 不存在")
        return

    try:
        # 删除整个 build 文件夹（包括所有子文件和子目录）
        shutil.rmtree(build_dir)
        # 重新创建一个空的 build 文件夹
        os.makedirs(build_dir)
        print(f"删除\"{build_dir}\"下的所有文件")
    except Exception as e:
        print(f"❌ 操作失败: {e}")
    

if __name__ == "__main__":
    current_dir = os.getcwd()  # 获取当前工作目录
    print(f"开始扫描目录: {current_dir}")
    delete_log_files(current_dir)
    clean_build_folder(current_dir)
    print("操作完成")