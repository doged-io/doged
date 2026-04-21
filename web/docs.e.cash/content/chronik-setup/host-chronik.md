---
title: Host Chronik
---

# Host Chronik

Use a reverse proxy such as [Apache](https://httpd.apache.org/) or [NGINX](https://www.nginx.com/) in front of Chronik.

## NGINX

[Install NGINX](https://docs.nginx.com/nginx/admin-guide/installing-nginx/installing-nginx-open-source/) — on Ubuntu:

```bash
sudo apt update
sudo apt install nginx
```

Example: serve Chronik under `/xec`:

```nginx
server {
    server_name chronik.yourapp.com;

    location /xec/ {
        proxy_pass http://127.0.0.1:8331/;
        proxy_set_header Host $http_host;
        add_header "Access-Control-Allow-Origin"  *;
        add_header "Access-Control-Allow-Methods" "GET, POST, OPTIONS, HEAD";
    }

    location /xec/ws {
        proxy_pass http://127.0.0.1:8331/ws;
        proxy_http_version 1.1;
        proxy_set_header Host $host;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "Upgrade";
    }
}
```

## HTTPS

Use Certbot with NGINX:

```bash
certbot --nginx -d chronik.yourapp.com
```

When prompted about redirects, choosing “yes” is usually best.
