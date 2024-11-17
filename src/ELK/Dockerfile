FROM sebp/elk

USER root

RUN rm /etc/logstash/conf.d/02-beats-input.conf
RUN rm /etc/logstash/conf.d/10-syslog.conf
RUN rm /etc/logstash/conf.d/11-nginx.conf
RUN rm /etc/logstash/conf.d/30-output.conf
RUN service logstash start