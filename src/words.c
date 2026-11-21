/******************************************************************************\
# C - words                                      #       Maximum Tension       #
################################################################################
#                                                #      -__            __-     #
# Teoman Deniz                                   #  :    :!1!-_    _-!1!:    : #
# maximum-tension.com                            #  ::                      :: #
#                                                #  :!:    : :: : :  :  ::::!: #
# +.....................++.....................+ #   :!:: :!:!1:!:!::1:::!!!:  #
# : C - Maximum Tension :: Create - 2026/11/19 : #   ::!::!!1001010!:!11!!::   #
# :---------------------::---------------------: #   :!1!!11000000000011!!:    #
# : License - GPL-3.0   :: Update - 2026/11/20 : #    ::::!!!1!!1!!!1!!!::     #
# +.....................++.....................+ #       ::::!::!:::!::::      #
\******************************************************************************/

/* **************************** [v] INCLUDES [v] **************************** */
#include "words.h"
/******* """"""""""
 * const char *const	WORDS_BUILT_IN[];
 * const int			WORDS_BUILT_IN_COUNT;
 */
/* **************************** [^] INCLUDES [^] **************************** */

/* UTF-8. WRITTEN THE USUAL WAY; UPPER AND LOWER CASE ARE TRIED TOO */
const char *const	WORDS_BUILT_IN[] = {
	/* COLOURS, TURKISH */
	"Beyaz", "Siyah", "Gri", "Kırmızı", "Mavi", "Yeşil", "Sarı", "Turuncu",
	"Mor", "Pembe", "Kahve", "Kahverengi", "Lacivert", "Bej", "Krem",
	"Ekru", "Vizon", "Bordo", "Füme", "Antrasit", "Haki", "Turkuaz", "Lila",
	"Hardal", "Somon", "Gümüş", "Altın", "Bakır", "Taba", "Camel", "Mint",
	"Indigo", "Petrol", "Fuşya", "Nar", "Kiremit", "Tarçın", "Kum", "Taş",
	"Toprak", "Zeytin", "Çağla", "Pudra", "Mürdüm", "Leylak", "Saks",
	"Kırık", "Açık", "Koyu", "Melanj", "Desenli", "Düz", "Çizgili",
	/* COLOURS, ENGLISH */
	"White", "Black", "Gray", "Grey", "Red", "Blue", "Green", "Yellow",
	"Orange", "Purple", "Pink", "Brown", "Navy", "Beige", "Cream", "Ecru",
	"Mink", "Burgundy", "Smoke", "Anthracite", "Khaki", "Turquoise",
	"Lilac", "Mustard", "Salmon", "Silver", "Gold", "Copper", "Tan",
	"Ivory", "Olive", "Stone", "Sand", "Light", "Dark", "Natural",
	"Charcoal", "Off", "Plain", "Striped",
	/* UNITS */
	"Top", "Adet", "Ad", "Kg", "Gr", "Gram", "Kilo", "Ton", "Metre", "Mt",
	"Cm", "Mm", "Litre", "Lt", "Paket", "Pkt", "Koli", "Kutu", "Kasa",
	"Çuval", "Rulo", "Bobin", "Çift", "Düzine", "Takım", "Set", "Parça",
	"Palet", "Şişe", "Torba", "Pcs", "Piece", "Pieces", "Box", "Roll",
	"Pair", "Dozen", "Meter", "Meters", "Yard", "Yards", "Unit", "Units",
	"Bag", "Pack", "Bale",
	/* HEADERS AND COMMON WORDS, TURKISH */
	"Toplam", "Genel", "Ara", "Miktar", "Fiyat", "Birim", "Tutar", "Tarih",
	"Ürün", "Kod", "Kodu", "Renk", "Rengi", "Açıklama", "Sıra", "Numara",
	"No", "Kdv", "Net", "Brüt", "Ağırlık", "Stok", "Depo", "Giriş", "Çıkış",
	"Kalan", "Sipariş", "Müşteri", "Firma", "Fatura", "İrsaliye", "Not",
	"Notlar", "Ad", "Soyad", "İsim", "Telefon", "Adres", "Şehir", "Ödeme",
	"Nakit", "Kredi", "Banka", "Borç", "Alacak", "Bakiye", "Gelir", "Gider",
	"Satış", "Alış", "İade", "Teslim", "Teslimat", "Hazır", "Bekliyor",
	"Tamam", "Var", "Yok", "Evet", "Hayır", "Kumaş", "İplik", "Pamuk",
	"Polyester", "Viskon", "Keten", "Yün", "İpek", "Likra", "Saten",
	"Kadife", "Denim", "Kot", "Örme", "Dokuma", "Astar", "Tül", "Penye",
	"Model", "Beden", "Boy", "En", "Ölçü", "Kalite", "Lot", "Parti",
	"Toptan", "Perakende", "Lira", "Dolar", "Euro", "TL",
	/* HEADERS AND COMMON WORDS, ENGLISH */
	"Total", "Subtotal", "Sum", "Quantity", "Qty", "Price", "Amount",
	"Date", "Item", "Product", "Code", "Color", "Colour", "Description",
	"Number", "Tax", "Weight", "Stock", "Order", "Customer", "Invoice",
	"Name", "Phone", "Address", "City", "Payment", "Cash", "Balance",
	"Notes", "Size", "Width", "Length", "Fabric", "Cotton", "Wool", "Silk",
	"Linen",
	/* TIME AND WORK (SHIFT LISTS, TIMETABLES), TURKISH AND ENGLISH */
	"Saat", "Dakika", "Süre", "Gün", "Hafta", "Ay", "Yıl", "Vardiya",
	"Mesai", "Başlangıç", "Bitiş", "Personel", "Çalışan", "Görev", "Bölüm",
	"Makine", "Hat", "Masa", "Oda", "Kat", "Yarım", "Tam", "Durum",
	"Sorumlu", "Plan", "Sabah", "Öğle", "Akşam", "Gece", "Time", "Hour",
	"Hours", "Minute", "Shift", "Start", "End", "Day", "Week", "Month",
	"Year", "Staff", "Table", "Room", "Half", "Full", "Part", "Status",
	"Morning", "Evening", "Night",
	/* MONTHS AND DAYS */
	"Ocak", "Şubat", "Mart", "Nisan", "Mayıs", "Haziran", "Temmuz",
	"Ağustos", "Eylül", "Ekim", "Kasım", "Aralık", "Pazartesi", "Salı",
	"Çarşamba", "Perşembe", "Cuma", "Cumartesi", "Pazar", "January",
	"February", "March", "April", "May", "June", "July", "August",
	"September", "October", "November", "December", "Monday", "Tuesday",
	"Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
};

const int			WORDS_BUILT_IN_COUNT = (int)(
	sizeof(WORDS_BUILT_IN) / sizeof(WORDS_BUILT_IN[0])
);
