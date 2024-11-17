FROM elastic/filebeat:8.15.4

COPY filebeat.yml /usr/share/filebeat/filebeat.yml

COPY dns.log /usr/share/filebeat/dns.log

USER root
RUN mkdir /usr/share/filebeat/dockerlogs

RUN chown root:root /usr/share/filebeat/filebeat.yml && \
chmod 644 /usr/share/filebeat/filebeat.yml

RUN chown root:root /usr/share/filebeat/dns.log && \
chmod 755 /usr/share/filebeat/dns.log
 
