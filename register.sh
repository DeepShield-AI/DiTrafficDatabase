#!/bin/bash
set -e

# ========= 基本配置 =========
ALIYUN_REG="registry.aliyuncs.com/google_containers"
PRIVATE_REG="10.10.10.108:5000"

# Kubernetes 版本（⚠️确保与你集群一致）
K8S_VERSION="v1.29.2"
COREDNS_VERSION="v1.11.1"
PAUSE_VERSION="3.8"
# FLANNEL_VERSION="v0.27.4"
METRICS_VERSION="v0.7.1"

# ========= 镜像清单 =========
# 格式：<阿里云镜像名>:<tag>|<私有仓库镜像名>:<tag>
IMAGES=(
  "kube-apiserver:${K8S_VERSION}|kube-apiserver:${K8S_VERSION}"
  "kube-controller-manager:${K8S_VERSION}|kube-controller-manager:${K8S_VERSION}"
  "kube-scheduler:${K8S_VERSION}|kube-scheduler:${K8S_VERSION}"
  "kube-proxy:${K8S_VERSION}|kube-proxy:${K8S_VERSION}"
  "pause:${PAUSE_VERSION}|pause:${PAUSE_VERSION}"
  "coredns:${COREDNS_VERSION}|coredns:${COREDNS_VERSION}"
  "flannel:${FLANNEL_VERSION}|flannel:${FLANNEL_VERSION}"
  "metrics-server:${METRICS_VERSION}|metrics-server:${METRICS_VERSION}"
)

# ========= 同步函数 =========
sync_image () {
  SRC_IMAGE="$1"
  DST_IMAGE="$2"

  echo "=============================================="
  echo ">> pull: ${ALIYUN_REG}/${SRC_IMAGE}"
  ctr images pull "${ALIYUN_REG}/${SRC_IMAGE}"

  echo ">> tag: ${PRIVATE_REG}/${DST_IMAGE}"
  ctr images tag "${ALIYUN_REG}/${SRC_IMAGE}" "${PRIVATE_REG}/${DST_IMAGE}"

  # echo ">> push: ${PRIVATE_REG}/${DST_IMAGE}"
  # docker push "${PRIVATE_REG}/${DST_IMAGE}"
}

# ========= 主流程 =========
echo ">>> 开始同步 Kubernetes 镜像到私有仓库"
echo ">>> 私有仓库: ${PRIVATE_REG}"
echo

for item in "${IMAGES[@]}"; do
  SRC="${item%%|*}"
  DST="${item##*|}"
  sync_image "$SRC" "$DST"
done

echo
echo ">>> DONE!"
