# 从完成版工程到自己的 GitHub 仓库

当前已经完成本地代码、测试、报告和分支合并。个人远程仓库还没创建，尚未推送。以下步骤用于完成最后的个人信息填写与上传。

## 1. 解压后先确认工程和历史

下载的 `vision-tracker-complete.zip` 包含项目目录和隐藏的 `.git` 文件夹，保留 `main`、`develop` 分支及提交历史。请保留 `.git`，不要在这个工程里重新初始化仓库。

在解压后的 `vision-tracker` 目录打开终端：

```bash
git status
git branch
git log --oneline --graph --all
```

默认位于 `main`，应看到 `develop` 分支和功能、测试、文档、合并提交。

如果自己从 GitHub 下载普通源码 ZIP，通常不会包含 `.git`；这里提供的完成版工程包专门保留了它。若提示不是 Git 仓库，先确认终端当前目录是否正确以及解压时是否保留隐藏文件。

## 2. 配置自己的提交身份

下面的中文内容需要替换为你的真实配置，不要原样复制占位内容：

```bash
git config user.name "你的名字"
git config user.email "你在GitHub使用的提交邮箱"
```

这是本仓库的提交署名配置，不代表已经登录 GitHub。现有提交使用了工作环境原本配置的 Git 身份；这些命令影响之后的新提交。

## 3. 创建一个空的个人仓库

1. 在浏览器登录自己的 GitHub 账号，打开 <https://github.com/new>。
2. 仓库名称可填 `vision-tracker`，所有者选择自己的账号。
3. 按验收要求设置可见性；如果使用私有仓库，需要让验收人有访问权限。
4. 不初始化 README、`.gitignore` 或许可证，创建一个空仓库。本地已经有源码和历史。
5. 创建完成后复制 HTTPS 仓库地址，形式为 `https://github.com/你的用户名/vision-tracker.git`。

本地 `upstream` 指向老师提供的原仓库；自己的仓库使用 `origin` 这个名字。运行：

```bash
git remote -v
```

首次添加自己的远程仓库：

```bash
git remote add origin https://github.com/你的用户名/vision-tracker.git
```

如果你之前已添加 `origin`，先查看它；需要修改地址时使用：

```bash
git remote set-url origin https://github.com/你的用户名/vision-tracker.git
```

不要把代码推送到 `upstream`，本作业要求提交到自己的仓库。

## 4. 补全报告，再通过 develop 合并

切换到已有的开发分支：

```bash
git switch develop
```

编辑根目录 `README.md`：填写姓名、学号、个人仓库链接。保留真实的测试结果和附加项完成情况。

保存后提交：

```bash
git diff -- README.md
git add README.md
git commit -m "docs: 补全作者信息与个人仓库地址"
git switch main
git merge --no-ff develop -m "merge: 合并报告个人信息"
```

- `git diff`：查看修改内容。
- `git add`：把文件放入本次待提交清单。
- `git commit`：保存一份本地历史记录。
- `git merge --no-ff`：合并并保留一条明确的合并记录。

本包中的代码和测试已经在 `develop` 开发并合并过。这一步处理的是你新补充的个人信息。

## 5. 在自己的电脑上编译与验证

Linux / WSL / macOS，安装好 C++17 编译器、CMake 和构建工具后，在工程根目录执行：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
./build/tracker
ctest --test-dir build --output-on-failure
```

如果使用 Ubuntu / Debian 的本机或 WSL，可先安装开发工具：

```bash
sudo apt update
sudo apt install build-essential cmake git
```

Windows Visual Studio 的多配置构建命令见根目录 README。`build/` 是本机生成的编译目录，已被 `.gitignore` 排除，无需上传。

预期：四组测试通过；主程序输出 28 次帧结果，其中 23 次接受、5 次拒绝。非法帧被拒绝正是题目的预期行为。

## 6. 登录并推送两个分支

当前云端环境的 GitHub 认证无效，未替你完成远程创建或推送。在自己的电脑上完成认证即可；不要在聊天中发送密码或令牌。

若已安装 GitHub CLI，可使用浏览器登录：

```bash
gh auth login --hostname github.com --git-protocol https --web
gh auth setup-git
```

也可以使用自己已配置好的 Git HTTPS 凭据管理器。认证完成后：

```bash
git push -u origin develop
git push -u origin main
```

`push` 才是真正上传，`commit` 只是保存到本地。`-u` 建立本地与远程分支的跟踪关系，方便后续直接运行 `git push`。

如果推送提示远程已经有其他提交，先检查远程内容和历史，不要直接强制覆盖。按本指南创建空仓库时，不应出现初始化 README 导致的历史冲突。

推送成功后，可把 README 中“尚未创建、尚未推送”的状态描述更新为真实情况，再按开发分支提交、合并和推送的流程保存。

## 7. 在网页上核对，再联系验收

- 网页的默认分支为 `main`，根目录展示项目报告。
- 分支列表中同时存在 `main` 和 `develop`。
- 能看到新增的四个类、测试文件和提交历史。
- `README.md` 中的姓名、学号、个人仓库链接已填写。
- `docs/assignment.md` 保留原题，`docs/STEP_BY_STEP.md` 是学习讲解。
- 自己能解释两种时间差、整帧拒绝、丢失恢复、继承和智能指针。

然后在 11 月 15 日截止前，带个人仓库链接联系学长验收。消息可自行写为：

> 学长好，我已完成 C++ 视觉目标管理与跟踪作业，仓库地址为：填写自己的仓库链接。代码和报告在 main 分支，保留了 develop 分支和开发提交，基础功能与本地自测已完成，请问什么时候方便验收？

这里只提供消息草稿，不会自动替你发送。

## 8. Markdown 怎么写、怎么预览

Markdown 是带简单排版标记的纯文本，文件扩展名是 `.md`。例如：

````markdown
# 一级标题

## 二级标题

这一行的 **文字是加粗的**。

- 第一项
- 第二项

代码中的变量可以写成 `dt_obs`。

```cpp
double velocity = distance / time;
```

[打开项目报告](../README.md)

| 项目 | 结果 |
| --- | --- |
| 自动测试 | 通过 |
````

标题和列表前后留空行，代码块用三个反引号围起来，`cpp` 用来标明 C++ 语法高亮。

VS Code 中打开 `.md` 文件，Windows/Linux 按 `Ctrl+Shift+V` 查看预览；依次按 `Ctrl+K`、`V` 可并排查看原文和预览。macOS 对应 `Command+Shift+V`，或依次按 `Command+K`、`V`。

查阅来源：[GitHub 官方 Markdown 语法](https://docs.github.com/en/get-started/writing-on-github/getting-started-with-writing-and-formatting-on-github/basic-writing-and-formatting-syntax)、[VS Code 官方 Markdown 文档](https://code.visualstudio.com/docs/languages/markdown)。
