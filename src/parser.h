#ifndef OSIC_PARSER_H
#define OSIC_PARSER_H

struct osic;
struct syntax;

struct syntax *
parser_parse(struct osic *osic);

#endif /* osic_PARSER_H */
