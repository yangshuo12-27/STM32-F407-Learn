# GitHub 推送与贡献图点亮教程

面向本项目（智慧果园 / STM32）从零到日常推送的完整说明。  
示例仓库：https://github.com/yangshuo12-27/STM32-learn

---

## 目录

1. [你需要准备什么](#1-你需要准备什么)
2. [安装与配置 Git](#2-安装与配置-git)
3. [GitHub 账号与邮箱（决定能不能点亮）](#3-github-账号与邮箱决定能不能点亮)
4. [在 GitHub 上创建仓库](#4-在-github-上创建仓库)
5. [把本地已有项目推上去（首次）](#5-把本地已有项目推上去首次)
6. [日常开发：改代码 → 提交 → 推送](#6-日常开发改代码--提交--推送)
7. [贡献图为什么会亮 / 不亮](#7-贡献图为什么会亮--不亮)
8. [登录与 Token（HTTPS 推送）](#8-登录与-tokenhttps-推送)
9. [常用命令速查](#9-常用命令速查)
10. [常见报错与处理](#10-常见报错与处理)
11. [建议的 .gitignore（STM32 项目）](#11-建议的-gitignorestm32-项目)
12. [推荐习惯](#12-推荐习惯)

---

## 1. 你需要准备什么

| 项目 | 说明 |
|------|------|
| GitHub 账号 | 浏览器打开 https://github.com 注册/登录 |
| Git 客户端 | Windows 推荐 [Git for Windows](https://git-scm.com/download/win)（含 Git Bash） |
| 本机项目 | 例如：`D:\Project\STM32F103C8T6\标准库\智慧果园` |
| 网络 | 能访问 github.com（校园网有时需代理） |

本教程用 **Git Bash** 或 **PowerShell** 均可，命令相同。

---

## 2. 安装与配置 Git

### 2.1 检查是否已安装

```bash
git --version
```

有版本号即可（如 `git version 2.4x.x`）。

### 2.2 全局用户名与邮箱（必做）

**每次 commit 都会带上这两个信息。邮箱必须和 GitHub 已验证邮箱一致，贡献图才会亮。**

```bash
git config --global user.name "yangshuo12-27"
git config --global user.email "yangshuo273277@gmail.com"
```

把上面的名字、邮箱换成你自己的。查看配置：

```bash
git config --global --list
```

只改当前仓库（不推荐新手）：

```bash
git config user.name "你的名字"
git config user.email "你的邮箱"
```

---

## 3. GitHub 账号与邮箱（决定能不能点亮）

1. 登录 GitHub → 右上角头像 → **Settings**
2. 左侧 **Emails**
3. 确认工作邮箱已添加，且状态为 **Verified（已验证）**
4. 建议勾选：
   - Keep my email addresses private（可选）
   - 若用隐私邮箱，则 `user.email` 应设为 GitHub 提供的 `xxx@users.noreply.github.com`

**原则：`git config` 里的 email = GitHub 已验证邮箱之一。**

---

## 4. 在 GitHub 上创建仓库

1. 右上角 **+** → **New repository**
2. Repository name：例如 `STM32-learn`
3. 公开 **Public** 或私有 **Private** 均可
4. **不要**勾选：
   - Add a README file
   - Add .gitignore
   - Choose a license  

   （空仓库更方便首次推送本地已有项目）
5. 点 **Create repository**
6. 页面会出现「…或从命令行中推送现有的仓库」，记下仓库地址，例如：

```text
https://github.com/yangshuo12-27/STM32-learn.git
```

---

## 5. 把本地已有项目推上去（首次）

### 5.1 进入项目目录

```bash
cd /d/Project/STM32F103C8T6/标准库/智慧果园
```

PowerShell：

```powershell
cd "D:\Project\STM32F103C8T6\标准库\智慧果园"
```

### 5.2 情况 A：项目里还没有 Git（从零初始化）

```bash
git init
git add .
git commit -m "初始提交：智慧果园 STM32 项目"
git branch -M main
git remote add origin https://github.com/yangshuo12-27/STM32-learn.git
git push -u origin main
```

### 5.3 情况 B：项目里已经有 Git（本仓库当前就是这样）

查看远程是否已绑定：

```bash
git remote -v
```

若已显示 `origin ... STM32-learn.git`，只需：

```bash
git branch -M main
git push -u origin main
```

若还没有 `origin`：

```bash
git remote add origin https://github.com/yangshuo12-27/STM32-learn.git
git branch -M main
git push -u origin main
```

若提示 `remote origin already exists`，可改地址：

```bash
git remote set-url origin https://github.com/yangshuo12-27/STM32-learn.git
```

### 5.4 成功标志

终端类似：

```text
To https://github.com/yangshuo12-27/STM32-learn.git
 * [new branch]      main -> main
branch 'main' set up to track 'origin/main'.
```

浏览器打开仓库主页，应能看到文件列表。

本项目此前已成功推送过一次，远程地址为：

https://github.com/yangshuo12-27/STM32-learn

---

## 6. 日常开发：改代码 → 提交 → 推送

这是「点亮贡献图」的常规操作。

```bash
# 1. 进入项目
cd /d/Project/STM32F103C8T6/标准库/智慧果园

# 2. 查看改了哪些文件
git status

# 3. 查看具体改动（可选）
git diff

# 4. 暂存要提交的文件
git add .
# 或只添加某个文件：
# git add Hardware/ESP8266.c

# 5. 本地提交（写清楚改了什么）
git commit -m "修复雷达 JSON 字段上报"

# 6. 推到 GitHub
git push
```

说明：

- `git add` + `git commit`：只发生在本机
- `git push`：上传到 GitHub，**贡献图通常在 push 之后才会计入**
- 提交信息建议写中文或英文均可，但要具体，例如：
  - 好：`更新雷达模拟器与 API 字段`
  - 避免：`改了点东西`、`asdf`、`111`

若没有改动可提交，`git commit` 会提示 nothing to commit。

---

## 7. 贡献图为什么会亮 / 不亮

GitHub 个人主页上的绿色格子，统计的是**符合条件的贡献**。

### 7.1 会计入的贡献类型

- Commit（提交）
- Opening an issue
- Opening / reviewing / merging a pull request

最常见的就是：**往默认分支 push commit**。

### 7.2 Commit 要变绿必须满足

1. Commit **作者邮箱**是你账号下已验证邮箱  
2. 提交在仓库的 **默认分支**（一般是 `main`）上，或符合 GitHub 规则的其他情况  
3. 仓库对你而言是「你的仓库 / 你有协作权限」等可计贡献场景  
4. 若是 **私有仓库**：主页 Contribution settings → 勾选 **Include private contributions**

### 7.3 常见不亮原因

| 原因 | 表现 | 处理 |
|------|------|------|
| 邮箱不一致 | push 成功但格子不绿 | 改 `user.email`，旧提交需改历史或新提交才绿 |
| 只 commit 没 push | 本地有记录，GitHub 没有 | `git push` |
| 推到非默认分支 | 只在 `dev` 上 | 合并进 `main`，或把默认分支改为该分支 |
| 私有仓库未勾选 | 有提交但不显示 | 打开 private contributions |
| 别人 fork 里乱提交 | 不一定算你的 | 在自己仓库或有权限的仓库提交 |

### 7.4 颜色深浅

同一天贡献越多，绿色越深。一天至少 1 次有效贡献即可点亮当天格子。

---

## 8. 登录与 Token（HTTPS 推送）

使用 `https://github.com/...` 推送时，GitHub **不再接受账户登录密码**，需要：

### 8.1 Personal Access Token（推荐新手）

1. GitHub → Settings → Developer settings  
2. **Personal access tokens** → Tokens (classic) 或 Fine-grained  
3. Generate new token  
4. 勾选权限：至少包含 **repo**（私有仓库必须）  
5. 生成后 **立刻复制保存**（离开页面可能再也看不到）  
6. `git push` 时：
   - Username：你的 GitHub 用户名  
   - Password：**粘贴 Token**（不是登录密码）

### 8.2 SSH（可选进阶）

生成密钥并加到 GitHub SSH keys 后，远程可改为：

```bash
git remote set-url origin git@github.com:yangshuo12-27/STM32-learn.git
```

之后 push 一般不必每次输密码（需本机 ssh-agent 正常）。

---

## 9. 常用命令速查

```bash
git status                 # 工作区状态
git log --oneline -10      # 最近 10 条提交
git remote -v              # 查看远程仓库地址
git branch -vv             # 本地分支及跟踪关系
git pull                   # 先拉远程更新（多人协作时）
git add .                  # 暂存全部改动
git commit -m "说明"       # 本地提交
git push                   # 推送到已跟踪的远程分支
git push -u origin main    # 首次推送并设置上游
```

撤销「还没 commit 的暂存」（慎用）：

```bash
git restore --staged 文件名
```

丢弃某文件未提交修改（不可恢复，慎用）：

```bash
git restore 文件名
```

---

## 10. 常见报错与处理

### 10.1 `Authentication failed` / `could not read Password`

Token 错误、过期或权限不足。重新生成 Token，再 push。

### 10.2 `remote: Repository not found`

- 仓库名/用户名写错  
- Token 没有该私有仓库权限  
- 没登录到正确账号  

检查：

```bash
git remote -v
```

### 10.3 `rejected` / `non-fast-forward`

远程比你新（例如网页上建了 README）。先拉再推：

```bash
git pull origin main --rebase
git push
```

### 10.4 `remote origin already exists`

```bash
git remote set-url origin https://github.com/yangshuo12-27/STM32-learn.git
```

### 10.5 中文路径 / 文件名乱码

Git Bash 一般可直接使用；PowerShell 路径建议加引号：

```powershell
cd "D:\Project\STM32F103C8T6\标准库\智慧果园"
```

### 10.6 文件太大推不上去

单个文件不宜超过 100MB。编译产物、库文件、数据包应加入 `.gitignore`，不要提交。

---

## 11. 建议的 .gitignore（STM32 项目）

在项目根目录创建 `.gitignore`，避免把编译垃圾推上去：

```gitignore
# Keil / MDK
*.o
*.d
*.crf
*.axf
*.htm
*.build_log.htm
*.dep
*.lnp
*.sct
*.map
*.lst
Objects/
Listings/
RTE/

# IDE / 系统
.vscode/
.idea/
*.bak
Thumbs.db
Desktop.ini

# Python
__pycache__/
*.pyc
.venv/
venv/

# 本地数据库 / 日志（按需）
*.db
*.log
```

添加后：

```bash
git add .gitignore
git commit -m "添加 .gitignore，忽略编译与缓存文件"
git push
```

---

## 12. 推荐习惯

1. **小步提交**：修一个问题、加一个功能就 commit 一次，信息写清楚。  
2. **先 status 再 commit**：避免把密码、`.env`、大文件误提交。  
3. **每天至少 push 一次**：想点亮贡献图，养成「当天工作结束再 push」的习惯。  
4. **不要刷假绿格子**：用脚本批量空 commit 意义不大，也不利于真实履历。  
5. **重要修改可写进 README**：别人（和未来的你）打开仓库能看懂怎么编译、怎么跑模拟器。

---

## 附录：本项目一键日常流程

```bash
cd /d/Project/STM32F103C8T6/标准库/智慧果园
git status
git add .
git commit -m "在这里写今天的改动说明"
git push
```

然后打开个人主页刷新贡献图即可。

仓库地址：https://github.com/yangshuo12-27/STM32-learn

---

*文档版本：2026-07 · 适用于 Windows + Git Bash / PowerShell + GitHub HTTPS*
