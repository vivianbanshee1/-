#!/usr/bin/env bash
# 一键提交脚本：保存版本 + 可选打tag
# 用法：
#   ./save_version.sh "本次提交说明"
#   ./save_version.sh "本次提交说明" --tag v1.0.0

set -euo pipefail

usage() {
  cat <<'USAGE'
使用方法:
  ./save_version.sh "提交说明" [--tag tag名称]

示例:
  ./save_version.sh "feat: 新增计分模块"
  ./save_version.sh "release: v1.0.0" --tag v1.0.0
USAGE
}

if [ "$#" -lt 1 ]; then
  usage
  exit 1
fi

MESSAGE="$1"
shift
TAG=""

if [ "$#" -gt 0 ]; then
  if [ "$1" = "--tag" ] || [ "$1" = "-t" ]; then
    if [ -z "${2:-}" ]; then
      echo "错误：--tag 后面需要提供标签名"
      usage
      exit 1
    fi
    TAG="$2"
  else
    echo "错误：不支持参数 $1"
    usage
    exit 1
  fi
fi

if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  echo "错误：当前目录不是 git 仓库"
  exit 1
fi

if ! git remote get-url origin >/dev/null 2>&1; then
  echo "错误：未配置 origin 远程仓库，请先执行："
  echo "  git remote add origin git@github.com:vivianbanshee1/vibe-coding.git"
  exit 1
fi

if git status --porcelain | grep -q .; then
  echo "检测到变更，开始提交..."
else
  echo "当前没有可提交内容"
  exit 0
fi

# 只提交源码/文档，不会提交已在 .gitignore 的文件

git add -A

git commit -m "$MESSAGE"

CURRENT_BRANCH="$(git branch --show-current)"
if [ -z "$CURRENT_BRANCH" ]; then
  CURRENT_BRANCH="main"
fi

git push -u origin "$CURRENT_BRANCH"

if [ -n "$TAG" ]; then
  git tag -a "$TAG" -m "$TAG"
  git push origin "$TAG"
  echo "标签已推送：$TAG"
fi

echo "✅ 已完成："
echo "  分支: $CURRENT_BRANCH"
echo "  提交信息: $MESSAGE"
if [ -n "$TAG" ]; then
  echo "  标签: $TAG"
fi
