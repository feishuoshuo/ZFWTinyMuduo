#!/usr/bin/env python3
import os
import subprocess
import shutil

# 若权限不足 linux$: chmod +x build.py     linux$:  sudo ./build.py

def run_command(command, cwd=None):
    """
    执行命令并检查返回值，如果命令失败则抛出异常。
    """
    try:
       # 将 stdout 和 stderr 设置为 None，让它们直接输出到终端
        result = subprocess.run(command, shell=True, check=True, cwd=cwd, text=True, stdout=None, stderr=None)
        print(f"命令执行成功: {command}")
        print(result.stdout)
    except subprocess.CalledProcessError as e:
        print(f"命令执行失败: {command}")
        print(e.stderr)
        raise

def main():
    # 获取当前脚本所在的根目录
    root_dir = os.path.dirname(os.path.abspath(__file__))
    build_dir = os.path.join(root_dir, "build")

    # 检查是否存在 build 目录，如果不存在则创建
    if not os.path.exists(build_dir):
        print("build 目录不存在，正在创建...")
        os.makedirs(build_dir)

    # 进入 build 目录
    os.chdir(build_dir)
    print(f"当前工作目录已切换到: {build_dir}")

    # 执行 make clean
    print("正在执行 make clean...")
    run_command("make clean")

    # 执行 cmake ..
    print("正在执行 cmake .....")
    run_command("cmake ..")

    # 执行 make
    print("正在执行 make...")
    run_command("make")

    # 回到根目录
    os.chdir(root_dir)
    print(f"当前工作目录已切换回: {root_dir}")

    # 创建目标目录（如果不存在）
    include_dir = "/usr/include/zfwmuduo"
    lib_dir = "/usr/lib"
    if not os.path.exists(include_dir):
        os.makedirs(include_dir)

    # 拷贝 net 和 base 目录下的所有头文件到 /usr/include/zfwmuduo/net,  /usr/include/zfwmuduo/base
    for directory in ["net", "base", "net/poller"]:
        src_dir = os.path.join(root_dir, directory)
        # 根据目录名确定目标子目录
        if directory.startswith("net/"):
            dst_subdir = os.path.join(include_dir, "net", os.path.relpath(directory, "net"))
        else:
            dst_subdir = os.path.join(include_dir, directory)
        
        # 确保目标子目录存在
        if not os.path.exists(dst_subdir):
            os.makedirs(dst_subdir)

        # 遍历源目录中的文件
        for filename in os.listdir(src_dir):
            if filename.endswith(".h"):
                src_file = os.path.join(src_dir, filename)
                dst_file = os.path.join(dst_subdir, filename)
                shutil.copy2(src_file, dst_file)
                print(f"拷贝文件: {src_file} -> {dst_file}")

    print("正在拷贝 so 库到 /usr/lib...")
    so_files = [f for f in os.listdir(os.path.join(root_dir, "lib")) if f.endswith(".so")]
    for so_file in so_files:
        src_so = os.path.join(os.path.join(root_dir, "lib"), so_file)
        dst_so = os.path.join(lib_dir, so_file)
        shutil.copy2(src_so, dst_so)
        print(f"拷贝文件: {src_so} -> {dst_so}")

    # 刷新动态库缓存
    print("正在刷新动态库缓存...")
    run_command("ldconfig")

    print("构建和安装完成！")

if __name__ == "__main__":
    main()