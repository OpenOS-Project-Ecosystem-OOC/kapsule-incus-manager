"""App container provisioning — deploy docker-compose apps inside Incus containers.

Inspired by incus-app-container and incus-compose.
Each app gets its own Incus container with Docker pre-installed and a systemd
service that watches docker-compose.yml for changes and reloads automatically.
"""

from __future__ import annotations

import textwrap
from typing import Any

import yaml


async def deploy_compose(incus: Any, config: dict[str, Any]) -> dict[str, Any]:
    """Create an Incus container configured to run a docker-compose app."""
    name        = config["name"]
    compose_src = config["compose"]
    image       = config.get("image", "images:ubuntu/24.04")
    _project    = config.get("project", "")  # reserved for future multi-project support
    disk_size   = config.get("disk_size", "20GB")
    profiles    = ["default", "nesting"]

    # Build the cloud-init user-data that installs Docker and sets up the
    # compose watcher service
    user_data = _build_cloud_init(name, compose_src)

    instance_config: dict[str, Any] = {
        "name": name,
        "type": "container",
        "source": {"type": "image", "alias": image},
        "profiles": profiles,
        "config": {
            "security.nesting": "true",
            "user.user-data": user_data,
        },
        "devices": {
            "root": {"type": "disk", "path": "/", "size": disk_size},
        },
    }

    if config.get("ip") and config["ip"] != "dhcp":
        instance_config["devices"]["eth0"] = {
            "type": "nic",
            "nictype": "bridged",
            "parent": config.get("bridge_name", "incusbr0"),
            "ipv4.address": config["ip"],
        }

    return await incus.create_instance(instance_config)


def _build_cloud_init(name: str, compose_src: str) -> str:
    """Generate cloud-init user-data that installs Docker and the compose watcher."""
    return textwrap.dedent(f"""\
        #cloud-config
        package_update: true
        packages:
          - docker.io
          - docker-compose-plugin
        write_files:
          - path: /appdata/docker-compose.yml
            content: |
{textwrap.indent(compose_src, "              ")}
          - path: /etc/systemd/system/compose-{name}.service
            content: |
              [Unit]
              Description=docker-compose app: {name}
              After=docker.service
              Requires=docker.service

              [Service]
              Type=simple
              WorkingDirectory=/appdata
              ExecStart=/usr/bin/docker compose up
              ExecStop=/usr/bin/docker compose down
              Restart=on-failure

              [Install]
              WantedBy=multi-user.target
          - path: /etc/systemd/system/compose-{name}-watcher.path
            content: |
              [Unit]
              Description=Watch docker-compose.yml for {name}

              [Path]
              PathModified=/appdata/docker-compose.yml

              [Install]
              WantedBy=multi-user.target
        runcmd:
          - systemctl daemon-reload
          - systemctl enable --now compose-{name}.service
          - systemctl enable --now compose-{name}-watcher.path
    """)


def convert_compose(compose_yaml: str) -> dict[str, Any]:
    """Convert a docker-compose.yaml to an incus-compose equivalent dict.

    Translates:
      - services → incus containers with docker: image source
      - ports    → proxy devices
      - volumes  → incus storage volumes
      - environment → container config keys
    """
    try:
        doc = yaml.safe_load(compose_yaml)
    except yaml.YAMLError as exc:
        return {"error": str(exc)}

    services = doc.get("services", {})
    top_volumes = doc.get("volumes", {})

    incus_services: dict[str, Any] = {}
    for svc_name, svc in services.items():
        proxies = []
        for port in svc.get("ports", []):
            parts = str(port).split(":")
            if len(parts) == 2:
                proxies.append({
                    "listen":  f"tcp:127.0.0.1:{parts[0]}",
                    "connect": f"tcp:0.0.0.0:{parts[1]}",
                })

        volumes = []
        for vol in svc.get("volumes", []):
            if isinstance(vol, str):
                parts = vol.split(":")
                if len(parts) >= 2 and parts[0] in top_volumes:
                    volumes.append({
                        "type":   "volume",
                        "source": parts[0],
                        "target": parts[1],
                        "read_only": len(parts) == 3 and "ro" in parts[2],
                    })

        incus_svc: dict[str, Any] = {
            "image": f"docker:{svc.get('image', '')}",
            "devices": {"proxies": proxies},
            "volumes": volumes,
        }
        if svc.get("container_name"):
            incus_svc["container_name"] = svc["container_name"]
        if svc.get("environment"):
            incus_svc["environment"] = svc["environment"]

        incus_services[svc_name] = incus_svc

    return {
        "services": incus_services,
        "volumes": {k: {"external": v.get("external", False) if v else False}
                    for k, v in top_volumes.items()},
    }
