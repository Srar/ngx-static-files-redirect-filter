# ngx-static-files-redirect-filter

## 概要

本模块可以将当前域下静态文件(css, js, img)重写URL来转发至另一台web服务器请求.

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

      # 重写URL后的域名 字符'$'会被替换为0~5的随机数 
      # 仅提供0~5随机数原因是如果域名都很分散反而会因为DNS解析降低一些访问速度
      # static_redirect_new_host "http://example111.com"; 

      # 是否携带源域名 默认:on
      # static_redirect_take_src_host on;

      # 源站返回的内容是否为UTF8内容 默认:on
      # static_redirect_utf8_content on;	

      # 是否将源域名base64编码 默认:off
      # static_redirect_base64_src_host off;		

      # 是否将源url path base64编码 默认:off
      # static_redirect_base64_src_url  off;		

      # 源域名与源url path之间的分割符号 默认: ""
      # static_redirect_split_tag "/";					
    }
}
```

## 配套转发

填坑中...
