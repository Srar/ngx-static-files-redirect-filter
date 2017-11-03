# ngx-static-files-redirect-filter

本模块可以在静态资源较多的网站上将当前域下静态文件(css, js, img)重写URL来转发至另一台web服务器请求.

## 案例

由于目前国内安置网站需要备案, 所以有很多站长选择了使用国外服务器来放网站.
不过大部分的国际出口链路属于吃紧状态, 即使使用了BBR或锐速等单边TCP加速软件效果也仅能提供一些效果.

当使用本模块之后, web server仅仅需要返回一个html文件, 其余静态文件列如css, js等您可以选择双边加速转发(kcptun)并缓存至您在境内的未备案(非80端口)的网站上.

当收到转发的静态资源后转发web server还能进一步处理, 列如: 将jpg, png格式转换为webp格式发送给支持的浏览器等. 或者在转发过程中重写为多个域例如: `1.static.example.com`, `2.static.example.com`, 这样也可以在非HTTP2情况下经可能提高静态资源并发加载上限.

这样就在单边TCP加速的基础上进一步提高网站的访问速度了.

## 思路来源

* 一起快网站加速器(已倒闭) 2014年

* [魔门云CacheMoment](https://www.cachemoment.com/) 2017年 - 

## 安装依赖

* [pcre](https://www.pcre.org/)

## 使用范例

```Nginx
server {
    listen       3000;
    server_name  _;

    proxy_buffering on;
    proxy_buffer_size 4k; 
    proxy_buffers 8 1M;
    proxy_busy_buffers_size 2M;
    proxy_max_temp_file_size 0;

    location / {
      proxy_pass http://example.com/;
      proxy_set_header Host "example.com";

      # 重写URL总开关
      # static_redirect on;                            

      # 重写URL后的域名 字符'$'会被替换为随机数 支持域名内带有多个'$'字符
      # static_redirect_new_host "http://example111.com";
    
      # 替换为随机数的区间, 值可以为0~9之间 默认: "0 3". 如果该值设置的区间很大可能会因为DNS解析导致速度降低.
      # static_redirect_new_host_ramdom 0 3

      # 限定最大处理的大小 如果超出该大小则会回退至普通返回 默认: 无限制
      # 如果设置该值过大可能会在特定场景导致DOS攻击
      # static_redirect_buffer_size_limit 128k;

      # 是否携带源域名 默认:on
      # static_redirect_take_src_host on;

      # 是否携带源请求path 默认:off
      # static_redirect_take_src_requesting_path on;

      # 源站返回的内容是否为UTF8内容 默认:on
      # static_redirect_utf8_content on;	

      # 是否将源域名base64编码 默认:off
      # static_redirect_base64_src_host off;	

      # 是否将源请求path base64编码 默认:off
      # static_redirect_base64_src_requesting_path off;		

      # 是否将重写的资源uri base64编码 默认:off
      # static_redirect_base64_src_url  off;		

      # 源域名,源path与重写的资源uri之间的分割符号 默认: ""
      # static_redirect_split_tag "/";					
    }
}
```

## 配套转发

填坑中...
