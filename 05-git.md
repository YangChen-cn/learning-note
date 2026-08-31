# Git 与 GitHub 基础

## 常用命令

```sh
git status                          # 看当前改动
git add -A                          # 全部暂存（新增/修改/删除都会加）
git commit -m "说明"                # 提交，信息写"做了什么"
git push                            # 推送到 GitHub
git log --oneline                   # 看提交历史（一行一条）
git branch --show-current           # 当前分支
```

## 身份配置（首次提交必做，否则报错）

```sh
git config user.name "YangChen"
git config user.email "yang666233@gmail.com"
# 在仓库内执行 → 只对本仓库生效；--global 则对全机器生效
```

## GitHub 免密推送（本机已配置好）

1. 安装 gh CLI：`sudo apt install -y /home/yang/gh_2.97.0_linux_amd64.deb`
2. 登录一次：`gh auth login` → 浏览器授权
3. 凭证生效：`gh auth setup-git` → git 自动走 gh 的 token，**以后 push 不再输密码**
4. 验证：`gh auth status` 显示 `Logged in to github.com account YangChen-cn`

## 本机仓库情况

| 位置 | Git 状态 | 远程 |
|---|---|---|
| `imx6u-edgepanel/` | ✅ 已连 | `github.com/YangChen-cn/my_linux_app.git`（main 分支） |
| `test/` | ❌ 不在 git 里 | 按用户要求，练习代码不纳入版本管理 |
| `learning-note/` | ❌ 不在 git 里 | 学习笔记，个人用 |

## 心得

- 提交信息写"做了什么 + 为什么"，别写"修改"这种废话
- `git status` 随时看，提交前先 status 确认范围对不对
- 推送是外发操作，别人仓库（或重要分支）上 push 前先确认
