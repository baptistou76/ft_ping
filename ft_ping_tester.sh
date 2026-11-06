#!/usr/bin/env bash

###############################################################################
# FT_PING - TESTS AUTOMATISES
#
# Lancement :
#   chmod +x ft_ping_tester.sh
#   ./ft_ping_tester.sh
#
# Le script doit etre lance avec Bash (pas avec "sh").
###############################################################################

set -u

FT_PING="${FT_PING:-./ft_ping}"
TIMEOUT_DURATION="${TIMEOUT_DURATION:-10}"
TMP_OUTPUT="$(mktemp)" || exit 1

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
RESET='\033[0m'

TOTAL=0
PASSED=0
FAILED=0

cleanup()
{
    rm -f "$TMP_OUTPUT"
}
trap cleanup EXIT INT TERM

section()
{
    printf '\n%b==================================================%b\n' "$CYAN" "$RESET"
    printf '%b%s%b\n' "$CYAN" "$1" "$RESET"
    printf '%b==================================================%b\n\n' "$CYAN" "$RESET"
}

if [[ ! -x "$FT_PING" ]]; then
    printf '%bERREUR :%b %s est introuvable ou non executable.\n' \
        "$RED" "$RESET" "$FT_PING"
    printf 'Compile d abord le projet avec make.\n'
    exit 1
fi

if ! command -v timeout >/dev/null 2>&1; then
    printf '%bERREUR :%b la commande timeout est introuvable.\n' "$RED" "$RESET"
    exit 1
fi

printf '%b==================================================%b\n' "$CYAN" "$RESET"
printf '%b              FT_PING TEST SUITE%b\n' "$CYAN" "$RESET"
printf '%b==================================================%b\n\n' "$CYAN" "$RESET"
printf 'Executable : %s\n' "$FT_PING"
printf 'Timeout    : %ss par test\n\n' "$TIMEOUT_DURATION"

printf '%bVerification des droits sudo...%b\n' "$YELLOW" "$RESET"
if ! sudo -v; then
    printf '%bImpossible d obtenir les droits sudo.%b\n' "$RED" "$RESET"
    exit 1
fi

# Modes :
#   reachable : le ping doit recevoir au moins une reponse (code 0)
#   either    : une reponse ou aucune reponse (codes 0 ou 1) sont acceptes
#   invalid   : la commande doit etre refusee (code non nul, sauf timeout)
#   help      : l aide doit se terminer avec le code 0
#   numeric   : le ping doit reussir et afficher une adresse IPv4 dans la reponse
run_test()
{
    local name="$1"
    local mode="$2"
    shift 2

    local status
    local success=0

    TOTAL=$((TOTAL + 1))
    printf '%b[%02d] %s%b\n' "$BLUE" "$TOTAL" "$name" "$RESET"

    sudo timeout --signal=TERM "${TIMEOUT_DURATION}s" \
        "$FT_PING" "$@" >"$TMP_OUTPUT" 2>&1
    status=$?

    case "$mode" in
        reachable)
            [[ "$status" -eq 0 ]] && success=1
            ;;
        either)
            [[ "$status" -eq 0 || "$status" -eq 1 ]] && success=1
            ;;
        invalid)
            [[ "$status" -ne 0 && "$status" -ne 124 && "$status" -ne 137 ]] \
                && success=1
            ;;
        help)
            [[ "$status" -eq 0 ]] && success=1
            ;;
        numeric)
            if [[ "$status" -eq 0 ]] &&
               grep -Eq 'bytes from [0-9]{1,3}(\.[0-9]{1,3}){3}' "$TMP_OUTPUT"; then
                success=1
            fi
            ;;
        *)
            printf '%bMode de test inconnu : %s%b\n' "$RED" "$mode" "$RESET"
            FAILED=$((FAILED + 1))
            return
            ;;
    esac

    if [[ "$success" -eq 1 ]]; then
        PASSED=$((PASSED + 1))
        printf '  %b[ OK ]%b code retour : %s\n' "$GREEN" "$RESET" "$status"
    else
        FAILED=$((FAILED + 1))
        printf '  %b[ ECHEC ]%b code retour : %s (mode attendu : %s)\n' \
            "$RED" "$RESET" "$status" "$mode"
    fi

    if [[ -s "$TMP_OUTPUT" ]]; then
        sed 's/^/       /' "$TMP_OUTPUT"
    else
        printf '       (aucune sortie)\n'
    fi
    printf '\n'
}

###############################################################################
# 1. FONCTIONNEMENT DE BASE ET RESOLUTION DNS
###############################################################################

section "1. FONCTIONNEMENT DE BASE ET DNS"

run_test "IPv4 loopback" reachable -c 2 127.0.0.1
run_test "Hostname localhost" reachable -c 2 localhost
run_test "Adresse IPv4 publique (Google DNS)" either -c 2 8.8.8.8
run_test "IP de documentation non routable" either -c 2 192.0.2.123
run_test "Resolution du hostname google.com" either -c 2 google.com
run_test "Nom DNS inexistant" invalid notarealhost.invalid
run_test "Nom DNS inexistant en verbose" invalid -v notarealhost.invalid
run_test "Deux destinations" invalid -c 2 127.0.0.1 localhost
run_test "Destination manquante" invalid
run_test "Argument supplementaire" invalid -c 2 127.0.0.1 extra_arg

###############################################################################
# 2. OPTIONS OBLIGATOIRES
###############################################################################

section "2. OPTIONS OBLIGATOIRES"

run_test "Aide -?" help -?
run_test "Mode verbose sur loopback" reachable -v -c 2 127.0.0.1
run_test "Verbose avec TTL bas (erreur ICMP possible)" either -v --ttl 1 -c 2 8.8.8.8

###############################################################################
# 3. OPTIONS STANDARD / EXTENSIONS DU PROJET
###############################################################################

section "3. OPTIONS STANDARD ET EXTENSIONS"

run_test "Count -c" reachable -c 3 127.0.0.1
run_test "Numeric avec IPv4" reachable -n -c 2 127.0.0.1
run_test "Numeric avec hostname" either -n -c 2 localhost
run_test "Taille payload 32" reachable -s 32 -c 2 127.0.0.1
run_test "Taille payload 56" reachable -s 56 -c 2 127.0.0.1
run_test "Taille payload 64" reachable -s 64 -c 2 127.0.0.1
run_test "Taille payload 1400" reachable -s 1400 -c 2 127.0.0.1
run_test "Taille negative invalide" invalid -s -1 127.0.0.1
run_test "Taille manquante" invalid -s

###############################################################################
# 4. BONUS : FLOOD
###############################################################################

section "4. BONUS - FLOOD (-f)"

run_test "Flood limite a 10 paquets" reachable -f -c 10 127.0.0.1
run_test "Flood avec payload" reachable -f -s 64 -c 10 127.0.0.1

###############################################################################
# 5. BONUS : PRELOAD
###############################################################################

section "5. BONUS - PRELOAD (-l)"

run_test "Preload de 5 paquets" reachable -l 5 -c 8 127.0.0.1
run_test "Preload de 3 paquets" reachable -l 3 -c 8 127.0.0.1
run_test "Preload zero" reachable -l 0 -c 2 127.0.0.1
run_test "Preload sans valeur" invalid -l

###############################################################################
# 6. BONUS : DEADLINE
###############################################################################

section "6. BONUS - DEADLINE (-w)"

run_test "Deadline 2 secondes vers IP de documentation" either -w 2 192.0.2.123
run_test "Deadline avec count" reachable -w 4 -c 2 127.0.0.1
run_test "Deadline zero invalide" invalid -w 0 127.0.0.1
run_test "Deadline sans valeur" invalid -w

###############################################################################
# 7. BONUS : LINGER / DELAI DE REPONSE
###############################################################################

section "7. BONUS - LINGER (-W)"

run_test "Linger 1 seconde, loopback" reachable -W 1 -c 2 127.0.0.1
run_test "Linger 1 seconde, destination non routable" either -W 1 -c 2 192.0.2.123
run_test "Linger zero invalide selon cette implementation" invalid -W 0 -c 2 127.0.0.1
run_test "Linger sans valeur" invalid -W

###############################################################################
# 8. BONUS : PATTERN
###############################################################################

section "8. BONUS - PATTERN (-p)"

run_test "Pattern ff" reachable -p ff -c 2 127.0.0.1
run_test "Pattern ff00" reachable -p ff00 -c 2 127.0.0.1
run_test "Pattern multi-octets" reachable -p aabbccdd -c 2 127.0.0.1
run_test "Pattern 12345678" reachable -p 12345678 -c 2 127.0.0.1
run_test "Pattern avec payload specifique" reachable -p 1234567890abcdef -s 64 -c 2 127.0.0.1
run_test "Pattern non hexadecimal" invalid -p xyz -c 2 127.0.0.1
run_test "Pattern longueur impaire" invalid -p abc -c 2 127.0.0.1
run_test "Pattern trop long" invalid -p 00112233445566778899aabbccddeeff00 -c 2 127.0.0.1
run_test "Pattern sans valeur" invalid -p

###############################################################################
# 9. BONUS : ROUTAGE
###############################################################################

section "9. BONUS - IGNORER LE ROUTAGE (-r)"

run_test "Ignore routing vers loopback" either -r -c 2 127.0.0.1
run_test "Ignore routing sans destination" invalid -r

###############################################################################
# 10. BONUS : TOS
###############################################################################

section "10. BONUS - TOS (-T NUM)"

run_test "TOS zero" reachable -T 0 -c 2 127.0.0.1
run_test "TOS decimal 16" reachable -T 16 -c 2 127.0.0.1
run_test "TOS hexadecimal 0x10" reachable -T 0x10 -c 2 127.0.0.1
run_test "TOS superieur a 255" invalid -T 256 -c 2 127.0.0.1
run_test "TOS non numerique" invalid -T abc -c 2 127.0.0.1
run_test "TOS sans valeur" invalid -T

###############################################################################
# 11. BONUS : TTL
###############################################################################

section "11. BONUS - TTL (--ttl)"

run_test "TTL 64" reachable --ttl 64 -c 2 127.0.0.1
run_test "TTL 1 vers destination externe" either --ttl 1 -c 2 8.8.8.8
run_test "TTL zero" invalid --ttl 0 -c 2 127.0.0.1
run_test "TTL 256" invalid --ttl 256 -c 2 127.0.0.1
run_test "TTL syntaxe avec egal" reachable --ttl=64 -c 2 127.0.0.1
run_test "TTL sans valeur" invalid --ttl

###############################################################################
# 12. BONUS : IP TIMESTAMP
###############################################################################

section "12. BONUS - IP TIMESTAMP (--ip-timestamp)"

run_test "Timestamp tsonly" reachable --ip-timestamp tsonly -c 2 127.0.0.1
run_test "Timestamp tsaddr" reachable --ip-timestamp tsaddr -c 2 127.0.0.1
run_test "Timestamp syntaxe avec egal" reachable --ip-timestamp=tsonly -c 2 127.0.0.1
run_test "Ancien mode tsandaddr refuse" invalid --ip-timestamp tsandaddr -c 2 127.0.0.1
run_test "Timestamp mode inconnu" invalid --ip-timestamp invalid -c 2 127.0.0.1
run_test "Timestamp sans mode" invalid --ip-timestamp

###############################################################################
# 13. COMBINAISONS
###############################################################################

section "13. COMBINAISONS D OPTIONS"

run_test "TTL + TOS + payload" reachable --ttl 64 -T 16 -s 64 -c 2 127.0.0.1
run_test "Pattern + preload" reachable -p ff00 -l 2 -c 5 127.0.0.1
run_test "Verbose + deadline" reachable -v -w 4 -c 2 127.0.0.1
run_test "Numeric + linger + count" reachable -n -W 1 -c 2 127.0.0.1

###############################################################################
# 14. ARGUMENTS INVALIDES ET OPTIONS NON PRISES EN CHARGE
###############################################################################

section "14. GESTION DES ERREURS"

run_test "Option inconnue" invalid -z 127.0.0.1
run_test "Count zero" invalid -c 0 127.0.0.1
run_test "Count negatif" invalid -c -1 127.0.0.1
run_test "Count sans valeur" invalid -c
run_test "Option -i non prevue par le sujet" invalid -i 1 -c 2 127.0.0.1
run_test "Option -h non prevue par le sujet" invalid -h
run_test "Aide longue --help" help --help

###############################################################################
# RAPPORT FINAL
###############################################################################

printf '\n%b==================================================%b\n' "$CYAN" "$RESET"
printf '%b                    BILAN FINAL%b\n' "$CYAN" "$RESET"
printf '%b==================================================%b\n' "$CYAN" "$RESET"
printf 'Total   : %d\n' "$TOTAL"
printf '%bReussis : %d%b\n' "$GREEN" "$PASSED" "$RESET"
printf '%bEchoues : %d%b\n' "$RED" "$FAILED" "$RESET"

if (( TOTAL > 0 )); then
    printf 'Taux de reussite : %d%%\n' "$((PASSED * 100 / TOTAL))"
fi

if (( FAILED == 0 )); then
    printf '\n%bTOUS LES TESTS SONT PASSES.%b\n' "$GREEN" "$RESET"
    exit 0
else
    printf '\n%bCERTAINS TESTS ONT ECHOUE.%b\n' "$RED" "$RESET"
    exit 1
fi
